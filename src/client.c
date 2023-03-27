#include "../include/config.h"
#include "../include/logger.h"
#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/ip.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

// Sends the client config file over TCP to the server
void pre_probing_c(struct Config *config) {
  char *server_ip = config->server_ip_addr;
  int dst_port = config->pp_port_tcp;
  int client_fd;
  struct sockaddr_in server_addr;
  char buffer[sizeof(struct Config) + 1] = {0};
  char server_res[1024] = {};

  // create socket
  if ((client_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    perror("socket failed");
    exit(EXIT_FAILURE);
  }

  // set server address
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(dst_port);

  if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) <= 0) {
    perror("invalid address");
    exit(EXIT_FAILURE);
  }

  // connect to server
  logger("[PRE-PROBING PHASE] Connecting to TCP Server");
  if (connect(client_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) <
      0) {
    perror("Failed connecting to server");
    exit(EXIT_FAILURE);
  }

  logger("[PRE-PROBING PHASE] Connected to server.");

  // config file data to be sent to server
  buffer[0] = CONFIG_FILE_RQ;
  memcpy(buffer + 1, config, sizeof(*config));

  // send message to server
  if (send(client_fd, buffer, sizeof(buffer), 0) < 0) {
    printf("Oops! Something went wrong sending config data");
    close(client_fd);
    return;
  } else {
    logger("[PRE-PROBING PHASE] Config data sent.");
  }

  // receive message from server
  read(client_fd, server_res, 1024);
  char msg[1024] = "[PRE-PROBING PHASE]";
  strcpy(msg + strlen("[PRE-PROBING PHASE] "), server_res);
  logger(msg);
}

int send_udp_packet(int server_fd, struct sockaddr_in *servaddr, char *payload,
                    int payload_size) {
  return (int)sendto(server_fd, payload, payload_size, 0,
                     (const struct sockaddr *)servaddr, sizeof(*servaddr));
}

void probing_c(struct Config *config) {
  char *server_ip = config->server_ip_addr;
  int inter_packet_delay_us = 300;
  int dst_port = config->dst_port_udp;
  int src_port = config->src_port_udp;
  int payload_size = config->payload_size;
  int train_size = config->udp_train_size;
  int inter_time_s = config->inter_time_s;
  int sock_fd;
  struct sockaddr_in serv_addr;

  if ((sock_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
    perror("[PROBING PHASE] Socket creation failed");
    exit(EXIT_FAILURE);
  }

  int optval = IP_PMTUDISC_DO;
  if (setsockopt(sock_fd, IPPROTO_IP, IP_MTU_DISCOVER, &optval,
                 sizeof(optval)) < 0) {
    perror("[PROBING_PHASE] setsockopt failed");
    exit(EXIT_FAILURE);
  }

  memset(&serv_addr, 0, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(dst_port);
  serv_addr.sin_addr.s_addr = inet_addr(server_ip);

  // Bind the socket to the source port
  struct sockaddr_in src_addr;
  memset(&src_addr, 0, sizeof(src_addr));
  src_addr.sin_family = AF_INET;
  src_addr.sin_port = htons(src_port);
  src_addr.sin_addr.s_addr = htonl(INADDR_ANY);

  if (bind(sock_fd, (const struct sockaddr *)&src_addr, sizeof(src_addr)) < 0) {
    perror("[PROBING PHASE] Error binding socket");
    exit(EXIT_FAILURE);
  }

  // Send low entropy packet train
  logger("[PROBING PHASE] Sending low-entropy packet train");
  for (int i = 0; i < train_size; i++) {
    // create low entropic payload
    char *payload_low = malloc(payload_size);
    uint16_t packet_id = htons((uint16_t)i);
    memset(payload_low, 0, payload_size);
    // insert packet_id
    memcpy(payload_low, &packet_id, sizeof(packet_id));
    // send packet
    int sent = send_udp_packet(sock_fd, &serv_addr, payload_low, payload_size);
    if (sent != payload_size) {
      printf("[PROBING PHASE] Expected bytes sent: %d; Actual bytes sent: %d\n",
             payload_size, sent);
    }
    // buffer time to prevent packet loss
    usleep(inter_packet_delay_us);
    free(payload_low);
  }

  // Wait for inter-measurement time
  logger("[PROBING PHASE] Sleeping inter-measurement time");
  sleep(inter_time_s);

  // Send high entropy packet train
  logger("[PROBING PHASE] Sending high-entropy packet train");
  for (int i = 0; i < train_size; i++) {
    // create high entropic payload
    char *payload_high = malloc(payload_size);
    uint16_t packet_id = htons((uint16_t)i);
    int random_fd = open("/dev/urandom", O_RDONLY);
    read(random_fd, payload_high, payload_size);
    close(random_fd);
    // insert packet_id
    memcpy(payload_high, &packet_id, sizeof(packet_id));
    // send packet
    int sent = send_udp_packet(sock_fd, &serv_addr, payload_high, payload_size);
    if (sent != payload_size) {
      printf("[PROBING PHASE] Expected bytes sent: %d; Actual bytes sent: %d\n",
             payload_size, sent);
    }
    // buffer time to prevent packet loss
    usleep(inter_packet_delay_us);
    free(payload_high);
  }

  // Done sending UDP packets
  logger("[PROBING PHASE] Sending high-entropy packet train");
  close(sock_fd);
}

char *receive_result(int sockfd, char *buffer, int buffer_size) {
  int n = recv(sockfd, buffer, buffer_size - 1, 0);
  if (n > 0) {
    buffer[n] = '\0';
    return buffer;
  }
  return "";
}

void post_probing_c(struct Config *config) {
  char *server_ip = config->server_ip_addr;
  int dst_port = config->pp_port_tcp;
  int server_fd;
  struct sockaddr_in server_addr;
  int buffer_size = 1024;
  char buffer[buffer_size];

  if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    perror("socket creation failed");
    exit(EXIT_FAILURE);
  }

  memset(&server_addr, 0, sizeof(server_addr));

  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(dst_port);

  if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) <= 0) {
    perror("inet_pton failed");
    exit(EXIT_FAILURE);
  }

  if (connect(server_fd, (const struct sockaddr *)&server_addr,
              sizeof(server_addr)) < 0) {
    perror("connect failed");
    exit(EXIT_FAILURE);
  }

  char *result = receive_result(server_fd, buffer, buffer_size);
  if (strcmp(result, "") == 0) {
    printf("[POST-PROBING PHASE] No response from server.\n");
  } else {
    printf("[COMP DETECT] %s\n", result);
  }
  close(server_fd);
}

void run_client(struct Config *config) {
  sleep(3); // give some time for server to start
  logger("[INFO] Init Pre-probing phase.");
  pre_probing_c(config); // <- run pre-probing
  logger("[INFO] Pre-probing phase completed.");
  logger("[INFO] Init Probing phase.");
  sleep(2);          // giving buffer time for server to start UDP server
  probing_c(config); // <- run probing
  logger("[INFO] Probing phase completed.");
  logger("[INFO] Init Post-probing phase.");
  sleep(2); // giving buffer time for server to re-start TCP server
  post_probing_c(config); // <- run post-probing
  logger("[INFO] Post-probing phase completed.");
}
