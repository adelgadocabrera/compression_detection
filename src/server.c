#include "../include/config.h"
#include "../include/logger.h"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#define THRESHOLD 100000

struct Config *recv_config(int server_fd, int client_fd) {
  char buffer[sizeof(struct Config) + 1] = {0};
  struct Config *client_config = malloc(sizeof(struct Config));

  if (recv(client_fd, buffer, sizeof(buffer), 0) < 0) {
    perror("[PRE-PROBING PHASE] Failed receiving message from client");
    close(client_fd);
    return NULL;
  }

  // In case we want to signal to shutdown server
  if (buffer[0] == SHUTDOWN_RQ) {
    printf("Server shutting down.");
    close(server_fd);
    close(client_fd);
    exit(0);
  }

  // Receive client config
  if (buffer[0] == CONFIG_FILE_RQ) {
    memcpy(client_config, buffer + 1, sizeof(struct Config));
    logger("[PRE-PROBING PHASE] Received config file from client.");

    // send message to client
    char *message = "Config file received!";
    send(client_fd, message, strlen(message), 0);
  } else {
    // handle unrecognized request
    char buf[1024];
    int size = snprintf(buf, sizeof(buf), "Received message: %s", buffer);
    if ((size_t)size < sizeof(buf)) {
      logger(buf);
    } else {
      logger("[PRE-PROBING PHASE] Received unrecognized message from client.");
    }

    // send message to client
    char *message = "Message unrecognized.";
    send(client_fd, message, strlen(message), 0);
  }

  if (buffer[0] == CONFIG_FILE_RQ) {
    return client_config;
  } else {
    return NULL;
  }
}

struct Config *pre_probing_s(int port) {
  int server_fd, client_fd; // file descriptors
  struct sockaddr_in server_addr, client_addr;
  int addr_len = sizeof(server_addr);

  // create socket
  if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
    perror("[PRE-PROBING PHASE] Failed creating server socket");
    exit(EXIT_FAILURE);
  }

  // set socket to re-use addr
  int optval = 1;
  if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) <
      0) {
    perror("[PRE-PROBING PHASE] Failed to set SO_REUSEADDR option");
    exit(EXIT_FAILURE);
  }

  // set server address
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_port = htons(port);

  // bind socket to server address
  if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) <
      0) {
    perror("[PRE-PROBING PHASE] Failed binding server socket");
    exit(EXIT_FAILURE);
  }

  // listen for incoming connections
  if (listen(server_fd, 3) < 0) {
    perror("[PRE-PROBING PHASE] Listening failed");
    exit(EXIT_FAILURE);
  }

  logger("[PRE-PROBING PHASE] Listening for incoming connections on port %d",
         port);

  // accept incoming connection
  if ((client_fd = accept(server_fd, (struct sockaddr *)&client_addr,
                          (socklen_t *)&addr_len)) < 0) {
    perror("[PRE-PROBING PHASE] Server accept new connection failed");
    close(client_fd);
    close(server_fd);
    return NULL;
  }

  logger("[PRE-PROBING PHASE] Accepted incoming connection from %s:%d.",
         inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

  struct Config *client_config = recv_config(server_fd, client_fd);
  close(client_fd);
  close(server_fd);
  return client_config;
}

void receive_udp_packet(int sockfd, struct sockaddr_in *cliaddr, char *payload,
                        int *payload_size, socklen_t *len) {
  *payload_size = recvfrom(sockfd, payload, *payload_size, 0,
                           (struct sockaddr *)cliaddr, len);
}

int probing_s(struct Config *client_config) {
  int dst_port = client_config->dst_port_udp;
  int train_size = client_config->udp_train_size;
  int payload_size = client_config->payload_size;
  int server_fd;
  struct sockaddr_in server_addr, client_addr;

  if ((server_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
    perror("[PROBING PHASE] Error creating UDP socket for pre_probing_phase");
    return 0;
  }

  memset(&server_addr, 0, sizeof(server_addr));
  memset(&client_addr, 0, sizeof(client_addr));

  // set server address
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_port = htons(dst_port);

  // bind socket to server address
  if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) <
      0) {
    perror("[PROBING PHASE] Failed creating server socket");
    exit(EXIT_FAILURE);
  }

  struct timespec start_low, end_low, start_high, end_high;
  char *payload = malloc(payload_size);
  socklen_t len;
  int counter = 0;

  // Receive low entropy packet train
  logger("[PROBING PHASE] Waiting for low-entropy packet train");
  for (int i = 0; i < train_size; i++) {
    recvfrom(server_fd, payload, payload_size, 0,
             (struct sockaddr *)&client_addr, &len);

    uint16_t packet_id;
    memcpy(&packet_id, payload, sizeof(packet_id));
    packet_id = ntohs(packet_id);

    if (packet_id == 0) {
      clock_gettime(CLOCK_MONOTONIC, &start_low);
    } else if (packet_id == train_size - 1) {
      clock_gettime(CLOCK_MONOTONIC, &end_low);
    }
    counter = counter + 1;
  }
  logger("[PROBING PHASE] Received %d/%d low-entropy packets", counter,
         train_size);

  // Receive high entropy packet train
  counter = 0;
  logger("[PROBING PHASE] Waiting for high-entropy packet train");
  for (int i = 0; i < train_size; i++) {
    recvfrom(server_fd, payload, payload_size, 0,
             (struct sockaddr *)&client_addr, &len);

    uint16_t packet_id;
    memcpy(&packet_id, payload, sizeof(packet_id));
    packet_id = ntohs(packet_id);

    if (packet_id == 0) {
      clock_gettime(CLOCK_MONOTONIC, &start_high);
    } else if (packet_id == train_size - 1) {
      clock_gettime(CLOCK_MONOTONIC, &end_high);
    }
    counter = counter + 1;
  }
  logger("[PROBING PHASE] Received %d/%d high-entropy packets", counter,
         train_size);

  free(payload);
  close(server_fd); // done receiving packets

  // calculate compression
  long delta_low = (end_low.tv_sec - start_low.tv_sec) * 1000000 +
                   (end_low.tv_nsec - start_low.tv_nsec) / 1000;
  long delta_high = (end_high.tv_sec - start_high.tv_sec) * 1000000 +
                    (end_high.tv_nsec - start_high.tv_nsec) / 1000;
  long delta_diff = delta_high - delta_low;
  logger("[PROBING PHASE] delta_high = %ld", delta_high);
  logger("[PROBING PHASE] delta_low = %ld", delta_low);
  logger("[PROBING PHASE] delta_diff = %ld", delta_diff);

  if (delta_diff > THRESHOLD) {
    logger("[PROBING PHASE] Compression detected!");
    return 1;
  } else {
    logger("[PROBING PHASE] No compression was detected.");
    return 0;
  }
}

void send_result(int sock_fd, int compression_detected) {
  char *message;
  if (compression_detected) {
    message = "Compression detected!";
  } else {
    message = "No compression was detected.";
  }
  send(sock_fd, message, strlen(message), 0);
}

void post_probing_s(int port, int has_compression) {
  int server_fd, client_fd;
  struct sockaddr_in server_addr, client_addr;
  socklen_t len;

  if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    perror("[POST-PROBING PHASE] Socket creation failed");
    exit(EXIT_FAILURE);
  }

  // set socket to re-use addr
  int optval = 1;
  if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) <
      0) {
    perror("[PRE-PROBING PHASE] Failed to set SO_REUSEADDR option");
    exit(EXIT_FAILURE);
  }

  memset(&server_addr, 0, sizeof(server_addr));
  memset(&client_addr, 0, sizeof(client_addr));

  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_port = htons(port);

  if (bind(server_fd, (const struct sockaddr *)&server_addr,
           sizeof(server_addr)) < 0) {
    perror("[POST-PROBING PHASE] Socket bind failed");
    exit(EXIT_FAILURE);
  }

  if (listen(server_fd, 1) < 0) {
    perror("[POST-PROBING PHASE] Listen failed");
    exit(EXIT_FAILURE);
  }

  logger("[POST-PROBING PHASE] Listening on port %d", port);
  len = sizeof(client_addr);
  if ((client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &len)) <
      0) {
    perror("[POST-PROBING PHASE] Accept failed");
    exit(EXIT_FAILURE);
  }

  send_result(client_fd, has_compression);
  logger("[POST-PROBING PHASE] Sent results to client! Closing.");

  close(client_fd);
  close(server_fd);
}

// Runs the server logic for the client/server compression detection
void run_server(int port) {
  logger("[INFO] Init Pre-probing phase.");
  struct Config *client_config = pre_probing_s(port); // <- run pre-probing
  if (client_config == NULL) {
    perror("Did not receive client config. Something went wrong");
    exit(EXIT_FAILURE);
  }
  logger("[INFO] Pre-probing phase completed.");
  logger("[INFO] Init Probing phase.");
  int has_compression = probing_s(client_config); // <- run probing
  free(client_config);
  logger("[INFO] Probing phase completed.");
  logger("[INFO] Init Post-probing phase.");
  post_probing_s(port, has_compression); // <- run post-probing
  logger("[INFO] Post-probing phase completed.");
}
