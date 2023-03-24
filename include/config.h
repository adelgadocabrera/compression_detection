#include <yaml.h>
#ifndef CONFIG_H
#define CONFIG_H

/** possible requests types between client / server */
#define SHUTDOWN_RQ 0x01
#define CONFIG_FILE_RQ 0x10

/** config struct and methods */
struct Config {
  char *mode;
  char *server_ip_addr;
  int src_port_udp;
  int dst_port_udp;
  int dst_port_tcp_hsyn;
  int dst_port_tcp_tsyn;
  int pp_port_tcp;
  int payload_size;
  int inter_time_s;
  int udp_train_size;
  int udp_ttl;
  int rst_timeout_s;
};

void init_config(struct Config *config);
struct Config *parse_config(struct Config *, char *filename);
void free_config(struct Config *config);
void print_config(struct Config *config);

#endif // CONFIG_H
