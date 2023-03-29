#include "../include/standalone.h"
#include "../include/config.h"
#include "../include/logger.h"
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

// 100ms in nanoseconds as fixed threshold above which compression is assumed to
// be enabled
#define THRESHOLD 100000000L

struct RstArgs {
  int rst_timeout_s;
  int rst_packets;
};

// This function creates a UDP socket and sets its time-to-live (TTL) value. It
// then sets the destination address and port using the provided parameters, and
// connects the socket to the destination address. If successful, it returns the
// socket file descriptor, and if any error occurs, it returns -1 and prints an
// error message.
int create_udp_socket(const char *dst_addr, int dst_port, int ttl) {
  int sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
  if (sock_fd < 0) {
    perror("Failed to create socket");
    return -1;
  }

  if (setsockopt(sock_fd, IPPROTO_IP, IP_TTL, &ttl, sizeof(ttl)) < 0) {
    perror("Failed to set TTL value");
    close(sock_fd);
    return -1;
  }

  struct sockaddr_in addr = {0};
  addr.sin_family = AF_INET;
  addr.sin_port = htons(dst_port);
  if (inet_pton(AF_INET, dst_addr, &addr.sin_addr) <= 0) {
    perror("Invalid address");
    close(sock_fd);
    return -1;
  }

  if (connect(sock_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
    perror("Failed to connect socket");
    close(sock_fd);
    return -1;
  }

  return sock_fd;
}

// This function sends a train of low-entropy packets over UDP to a specified
// destination address and port, with a specified time-to-live (TTL) value,
// train size, payload size, and inter-packet delay.
void send_udp_low_entropy_packet_train(const char *dst_addr, int dst_port,
                                       int ttl, int train_size,
                                       int payload_size,
                                       int inter_packet_delay_us) {
  char payload[payload_size];
  memset(payload, 0, payload_size);

  int sock_fd = create_udp_socket(dst_addr, dst_port, ttl);
  if (sock_fd < 0) {
    return;
  }

  for (int i = 0; i < train_size; i++) {
    uint16_t packet_id = htons(i);
    memcpy(payload, &packet_id, sizeof(packet_id));
    send(sock_fd, payload, payload_size, 0);
    if (i < train_size - 1) {
      usleep(inter_packet_delay_us);
    }
  }

  close(sock_fd);
}

// This function sends a high-entropy packet train over UDP to a specified
// destination address and port using random bytes generated from /dev/urandom.
// The payload size, number of packets in the train, and inter-packet delay can
// be specified as parameters, as well as the time-to-live (TTL) value for the
// packets.
void send_udp_high_entropy_packet_train(const char *dst_addr, int dst_port,
                                        int ttl, int train_size,
                                        int payload_size,
                                        int inter_packet_delay_us) {
  unsigned char payload[payload_size];
  FILE *urandom = fopen("/dev/urandom", "r");

  if (!urandom) {
    perror("Error opening /dev/urandom");
    return;
  }

  int sock_fd = create_udp_socket(dst_addr, dst_port, ttl);
  if (sock_fd < 0) {
    fclose(urandom);
    return;
  }

  for (int i = 0; i < train_size; i++) {
    uint16_t packet_id = htons(i);
    memcpy(payload, &packet_id, sizeof(packet_id));

    // Fill the rest of the payload with random bytes from /dev/urandom
    if (fread(payload + 2, 1, payload_size - 2, urandom) !=
        (size_t)payload_size - 2) {
      perror("Error reading from /dev/urandom");
      fclose(urandom);
      close(sock_fd);
      return;
    }

    send(sock_fd, payload, payload_size, 0);
    // helps prevent packet loss although in this case we are not trying to
    // read the udp packets in a server so we don't really need this
    if (i < train_size - 1) {
      usleep(inter_packet_delay_us);
    }
  }

  fclose(urandom);
  close(sock_fd);
}

// This function calculates the TCP checksum for a given buffer of unsigned
// shorts. It iterates over the buffer two bytes at a time, accumulating the sum
// of each pair of bytes. If the buffer has an odd number of bytes, it also adds
// the last byte to the sum. Then it performs a carry operation on the sum, and
// takes the one's complement of the result to obtain the final checksum value.
uint16_t tcp_checksum(unsigned short *buf, int len) {
  unsigned long sum = 0;
  while (len > 1) {
    sum += *buf++;
    len -= 2;
  }
  if (len == 1) {
    sum += *(unsigned char *)buf;
  }
  sum = (sum >> 16) + (sum & 0xffff);
  sum += (sum >> 16);
  return (uint16_t)~sum;
}

// This function builds a TCP SYN packet with a specified source IP address,
// destination IP address, source port, destination port, and time-to-live
// (TTL). It first fills in the IP header and then the TCP header with the SYN
// flag set. It also sets the window size and computes the TCP checksum. The
// function takes a pointer to the packet buffer as an argument and modifies it
// directly.
void build_tcp_syn_packet(char *packet, char *src_ip, char *dst_ip,
                          uint16_t src_port, uint16_t dst_port, int ttl) {
  int window_size = 64495; // saw this in packets in wireshark

  // Fill in IP header
  struct iphdr *iph = (struct iphdr *)packet;
  iph->ihl = 5;
  iph->version = 4;
  iph->tos = 0;
  iph->tot_len = htons(sizeof(struct iphdr) + sizeof(struct tcphdr));
  iph->id = htons(0);
  iph->frag_off = htons(0);
  iph->ttl = ttl;
  iph->protocol = IPPROTO_TCP;
  iph->saddr = inet_addr(src_ip);
  iph->daddr = inet_addr(dst_ip);
  iph->check = 0;

  // Fill in TCP header
  struct tcphdr *tcph = (struct tcphdr *)(packet + sizeof(struct iphdr));
  tcph->source = htons(src_port);
  tcph->dest = htons(dst_port);
  tcph->seq = htonl(0);
  tcph->ack_seq = 0;
  tcph->doff = 5;
  tcph->fin = 0;
  tcph->syn = 1;
  tcph->rst = 0;
  tcph->psh = 0;
  tcph->ack = 0;
  tcph->urg = 0;
  tcph->window = htons(window_size);
  tcph->check = 0;
  tcph->urg_ptr = 0;

  // Fill in TCP pseudo-header
  struct pseudo_tcp_header {
    uint32_t src_addr;
    uint32_t dst_addr;
    uint8_t reserved;
    uint8_t protocol;
    uint16_t tcp_length;
  } psh;
  psh.src_addr = inet_addr(src_ip);
  psh.dst_addr = inet_addr(dst_ip);
  psh.reserved = 0;
  psh.protocol = IPPROTO_TCP;
  psh.tcp_length = htons(sizeof(struct tcphdr));

  // Compute TCP checksum
  char buf[sizeof(psh) + sizeof(struct tcphdr)];
  memcpy(buf, &psh, sizeof(psh));
  memcpy(buf + sizeof(psh), tcph, sizeof(struct tcphdr));
  tcph->check = tcp_checksum((uint16_t *)buf, sizeof(buf));
}

// This function listens for incoming RST packets on a raw socket and records
// the timestamps of the received packets. It continues listening until it
// receives a specified number of RST packets or a timeout is reached. If enough
// RST packets are received, it calculates the time differences between the
// first and second packet and the third and fourth packet, and compares them to
// a threshold value. If the time difference is greater than the threshold, it
// assumes that compression is being used. The function prints a message
// indicating whether compression was detected or not. If not enough RST packets
// are received, the function prints a message indicating that fact.
void *listen_for_rst_packets(void *args) {
  struct RstArgs *rst_args = (struct RstArgs *)args;
  int rst_timeout_s = rst_args->rst_timeout_s;
  int rst_packets = rst_args->rst_packets;

  // Create raw socket
  int sock = socket(AF_INET, SOCK_RAW, IPPROTO_TCP);
  if (sock < 0) {
    perror("socket");
    exit(EXIT_FAILURE);
  }

  // Listen for incoming packets
  char buf[65535]; // 2**16 - 1
  int packets_received = 0;
  struct timespec timestamps[rst_packets];

  while (packets_received < rst_packets) {
    fd_set read_fds;
    FD_ZERO(&read_fds);
    FD_SET(sock, &read_fds);
    struct timeval timeout;
    timeout.tv_sec = rst_timeout_s;
    timeout.tv_usec = 0;

    int ready_fds = select(sock + 1, &read_fds, NULL, NULL, &timeout);
    if (ready_fds < 0) {
      perror("[STANDALONE] Listening to RST select");
      exit(EXIT_FAILURE);
    } else if (ready_fds == 0) {
      printf("[STANDALONE] [ERROR] Timeout reached, stopping the listener.\n");
      break;
    }

    memset(buf, 0, sizeof(buf));
    struct sockaddr_in src_addr;
    socklen_t src_addr_len = sizeof(src_addr);
    int num_bytes = recvfrom(sock, buf, sizeof(buf), 0,
                             (struct sockaddr *)&src_addr, &src_addr_len);
    if (num_bytes < 0) {
      perror("recvfrom");
      exit(EXIT_FAILURE);
    }

    // Check for RST packet
    struct iphdr *iph = (struct iphdr *)buf;
    struct tcphdr *tcph = (struct tcphdr *)(buf + sizeof(struct iphdr));
    if (iph->protocol == IPPROTO_TCP && tcph->rst) {
      logger("[STANDALONE] Received RST packet from %s:%u",
             inet_ntoa(src_addr.sin_addr), ntohs(tcph->source));

      clock_gettime(CLOCK_MONOTONIC, &timestamps[packets_received]);
      packets_received++;
    }
  }

  if (packets_received == rst_packets) {
    // Calculate delta time when all RST packets have been received
    double delta_low = (timestamps[1].tv_sec - timestamps[0].tv_sec) * 1e9 +
                       (timestamps[1].tv_nsec - timestamps[0].tv_nsec);
    double delta_high = (timestamps[3].tv_sec - timestamps[2].tv_sec) * 1e9 +
                        (timestamps[3].tv_nsec - timestamps[2].tv_nsec);
    double delta_diff = delta_high - delta_low;

    int to_ms = 1000000;
    logger("[STANDALONE] delta_low = %.2f ms", delta_low / to_ms);
    logger("[STANDALONE] delta_high = %.2f ms", delta_high / to_ms);
    logger("[STANDALONE] delta_diff = %.2f ms", delta_diff / to_ms);

    if (delta_diff > THRESHOLD) {
      printf("[STANDALONE] Compression detected!\n");
    } else {
      printf("[STANDALONE] No compression was detected.\n");
    }
  } else {
    printf("[STANDALONE] Not enough RST packets received.\n");
  }

  // Close socket
  close(sock);
  return NULL;
}

// Sends a TCP SYN packet to initiate a TCP connection with a destination host.
// The function creates a raw socket and sets the IP_HDRINCL option to specify
// that the IP header will be included in the packet data. Then it builds a TCP
// SYN packet using the build_tcp_syn_packet function and sets the destination
// address. Finally, it sends the packet using sendto function and closes the
// socket. The parameters of the function include the source and destination IP
// addresses and port numbers, as well as the TTL value for the packet.
void send_tcp_syn_packet(char *src_ip, char *dst_ip, unsigned short src_port,
                         unsigned short dst_port, int ttl) {
  // Create raw socket
  int sock = socket(AF_INET, SOCK_RAW, IPPROTO_RAW);
  if (sock < 0) {
    perror("socket");
    exit(EXIT_FAILURE);
  }

  // Set socket options
  int optval = 1;
  if (setsockopt(sock, IPPROTO_IP, IP_HDRINCL, &optval, sizeof(optval)) < 0) {
    perror("setsockopt");
    exit(EXIT_FAILURE);
  }

  // Build TCP SYN packet
  char packet[65535];
  memset(packet, 0, sizeof(packet));
  build_tcp_syn_packet(packet, src_ip, dst_ip, src_port, dst_port, ttl);

  // Set destination address
  struct sockaddr_in dest_addr;
  dest_addr.sin_family = AF_INET;
  dest_addr.sin_addr.s_addr = inet_addr(dst_ip);
  dest_addr.sin_port = htons(dst_port);

  // Send TCP SYN packet
  int num_bytes =
      sendto(sock, packet, sizeof(struct iphdr) + sizeof(struct tcphdr), 0,
             (struct sockaddr *)&dest_addr, sizeof(dest_addr));
  if (num_bytes < 0) {
    perror("sendto");
    exit(EXIT_FAILURE);
  }

  // Close socket
  close(sock);
}

// The run_standalone function is the main function that sends packets and
// listens for RST packets in order to detect compression. It sends a sequence
// of packets, including a TCP SYN packet to two different ports, followed by a
// train of low or high entropy UDP packets, and then another TCP SYN packet to
// the second port. It also starts a thread to listen for RST packets and waits
// for it to finish.
void run_standalone(struct Config *config) {
  int src_port = config->pp_port_tcp;
  char *src_ip = "127.0.0.1";
  char *dst_ip = config->server_ip_addr;
  int port_x = config->dst_port_tcp_hsyn;
  int port_y = config->dst_port_tcp_tsyn;
  int udp_dst_port = config->dst_port_udp;
  int train_size = config->udp_train_size;
  int payload_size = config->payload_size;
  int ttl = config->udp_ttl;
  int rst_timeout_s = config->rst_timeout_s;
  int inter_packet_delay_us = 300;
  struct RstArgs rst_args;
  pthread_t rst_thread;

  rst_args.rst_timeout_s = rst_timeout_s;
  rst_args.rst_packets = 4;

  // Start listening thread for RST packets
  if (pthread_create(&rst_thread, NULL, listen_for_rst_packets, &rst_args) !=
      0) {
    perror("pthread_create");
    exit(EXIT_FAILURE);
  }

  usleep(1000); // give some buffer time

  // - Send TCP SYN packet to port x
  // - Send train of udp low entropy packets
  // - Send TCP SYN packet to port y
  logger("[STANDALONE] Sending SYN packet to port_x %d", port_x);
  send_tcp_syn_packet(src_ip, dst_ip, src_port, port_x, ttl);
  logger("[STANDALONE] Sending low entropy UDP packet train");
  send_udp_low_entropy_packet_train(dst_ip, udp_dst_port, ttl, train_size,
                                    payload_size, inter_packet_delay_us);
  logger("[STANDALONE] Low entropy UDP packet train sent");
  logger("[STANDALONE] Sending SYN packet to port_y %d", port_y);
  send_tcp_syn_packet(src_ip, dst_ip, src_port, port_y, ttl);

  logger("[STANDALONE] Waiting time between packet trains...");
  sleep(5);

  // - Send TCP SYN packet to port x
  // - Send train of udp low entropy packets
  // - Send TCP SYN packet to port y
  logger("[STANDALONE] Sending SYN packet to port_x %d", port_x);
  send_tcp_syn_packet(src_ip, dst_ip, src_port, port_x, ttl);
  logger("[STANDALONE] Sending high entropy UDP packet train");
  send_udp_high_entropy_packet_train(dst_ip, udp_dst_port, ttl, train_size,
                                     payload_size, inter_packet_delay_us);
  logger("[STANDALONE] High entropy UDP packet train sent");
  logger("[STANDALONE] Sending SYN packet to port_y %d", port_y);
  send_tcp_syn_packet(src_ip, dst_ip, src_port, port_y, ttl);

  // Wait for listening thread to finish
  if (pthread_join(rst_thread, NULL) != 0) {
    perror("[STANDALONE] [ERROR] pthread_join first batch");
    exit(EXIT_FAILURE);
  }
}
