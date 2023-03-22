#include "../include/config.h"
#include "../include/logger.h"
#include <arpa/inet.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

void send_syn_packet(int sock_fd, struct sockaddr_in *addr, int src_port,
                     int dst_port, int ttl) {
  char packet[512] = {0};
  struct iphdr *ip_header = (struct iphdr *)packet;
  struct tcphdr *tcp_header = (struct tcphdr *)(packet + sizeof(struct iphdr));

  // define ip headers
  ip_header->ihl = 5;
  ip_header->version = 4;
  ip_header->tos = 0;
  ip_header->tot_len = htons(sizeof(struct iphdr) + sizeof(struct tcphdr));
  ip_header->id = htons(0);
  ip_header->frag_off = 0;
  ip_header->ttl = ttl;
  ip_header->protocol = IPPROTO_TCP;
  ip_header->check = 0;
  ip_header->saddr = addr->sin_addr.s_addr;
  ip_header->daddr = addr->sin_addr.s_addr;

  // define tcp headers
  tcp_header->source = htons(src_port);
  tcp_header->dest = htons(dst_port);
  tcp_header->seq = htonl(1);
  tcp_header->ack_seq = 0;
  tcp_header->doff = sizeof(struct tcphdr) / 4;
  tcp_header->syn = 1;
  tcp_header->window = htons(65535);
  tcp_header->check = 0;
  tcp_header->urg_ptr = 0;

  sendto(sock_fd, packet, ip_header->tot_len, 0, (struct sockaddr *)addr,
         sizeof(struct sockaddr_in));
}

void send_udp_packet_train(int sock_fd, struct sockaddr_in *addr,
                           int train_size, int payload_size,
                           int inter_packet_delay_us) {
  for (int i = 0; i < train_size; i++) {
    char payload[payload_size];
    memset(payload, 0, payload_size);
    sendto(sock_fd, payload, payload_size, 0, (struct sockaddr *)addr,
           sizeof(struct sockaddr_in));
    usleep(inter_packet_delay_us);
  }
}

void run_standalone(struct Config *config) {
  const char *server_ip = config->server_ip_addr;
  int train_size = config->udp_train_size;
  int payload_size = config->payload_size;
  int port_x = config->dst_port_tcp_hsyn;
  int port_y = config->dst_port_tcp_tsyn;
  int src_port = config->pp_port_tcp;
  int ttl = config->udp_ttl;
  int inter_packet_delay_us = 200;
  int rst_timeout_s = config->rst_timeout_s;

  logger("[STANDALONE] Creating socket");
  int sock_fd = socket(AF_INET, SOCK_RAW, IPPROTO_TCP);
  if (sock_fd < 0) {
    perror("Error creating socket");
    return;
  }

  struct sockaddr_in server_addr;
  memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(src_port);
  inet_aton(server_ip, &server_addr.sin_addr);

  // Send SYN packet to port x
  logger("[STANDALONE] Sending SYN packet to port %d", port_x);
  send_syn_packet(sock_fd, &server_addr, src_port, port_x, ttl);

  // Send UDP packet train
  logger("[STANDALONE] Sending UDP packet train");
  send_udp_packet_train(sock_fd, &server_addr, train_size, payload_size,
                        inter_packet_delay_us);
  logger("[STANDALONE] UDP packet train sent");

  // Send SYN packet to port y
  logger("[STANDALONE] Sending SYN packet to %d", port_y);
  send_syn_packet(sock_fd, &server_addr, src_port, port_y, ttl);

  // Receive RST packets and calculate time difference
  logger("[STANDALONE] Receiving RST packets");
  struct timespec start_time, end_time;
  struct timeval timeout;
  fd_set read_fds;
  int result;
  ssize_t received_bytes;
  char buf[1024];
  int rst_received = 0;

  while (rst_received < 2) {
    FD_ZERO(&read_fds);
    FD_SET(sock_fd, &read_fds);

    timeout.tv_sec = rst_timeout_s;
    timeout.tv_usec = 0;

    result = select(sock_fd + 1, &read_fds, NULL, NULL, &timeout);

    if (result < 0) {
      perror("select");
      break;
    } else if (result == 0) {
      printf("Failed to detect due to insufficient information.\n");
      break;
    } else {
      received_bytes = recv(sock_fd, buf, sizeof(buf), 0);
      if (received_bytes > 0) {
        struct ip *ip_hdr = (struct ip *)buf;
        struct tcphdr *tcp_hdr = (struct tcphdr *)(buf + ip_hdr->ip_hl * 4);

        if (tcp_hdr->rst) {
          rst_received++;
          if (rst_received == 1) {
            clock_gettime(CLOCK_MONOTONIC, &start_time);
          } else if (rst_received == 2) {
            clock_gettime(CLOCK_MONOTONIC, &end_time);
          }
        }
      }
    }
  }

  if (rst_received == 2) {
    long time_diff = (end_time.tv_sec - start_time.tv_sec) * 1000000 +
                     (end_time.tv_nsec - start_time.tv_nsec) / 1000;
    printf("Time difference between RST packets: %ld microseconds\n",
           time_diff);
  }

  close(sock_fd);
}
