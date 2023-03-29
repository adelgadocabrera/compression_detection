#include "../include/config.h"
#include "../include/logger.h"
#include <stdbool.h>
#include <stdio.h>
#include <yaml.h>

// Initialize fields to default values
void init_config(struct Config *config) {
  config->mode = NULL;
  config->server_ip_addr = NULL;
  config->src_port_udp = 0;
  config->dst_port_udp = 0;
  config->dst_port_tcp_hsyn = 0;
  config->dst_port_tcp_tsyn = 0;
  config->pp_port_tcp = 0;
  config->payload_size = 0;
  config->inter_time_s = 0;
  config->udp_train_size = 0;
  config->udp_ttl = 0;
  config->rst_timeout_s = 0;
}

// Parse yaml file
struct Config *parse_config(struct Config *config, char *filename) {
  FILE *file = fopen(filename, "r");
  if (!file) {
    printf("Failed to open file\n");
    return NULL;
  }

  yaml_parser_t parser;
  yaml_event_t event;
  int done = 0;

  yaml_parser_initialize(&parser);
  yaml_parser_set_input_file(&parser, file);

  while (!done) {
    if (!yaml_parser_parse(&parser, &event)) {
      printf("Failed to parse YAML file\n");
      free_config(config);
      return NULL;
    }

    // Check for property in yaml file
    // - is there a cleaner/extendable way?
    switch (event.type) {
    case YAML_SCALAR_EVENT:
      if (strcmp((char *)event.data.scalar.value, "mode") == 0) {
        yaml_parser_parse(&parser, &event);
        config->mode = malloc(strlen((char *)event.data.scalar.value) + 1);
        if (!config->mode) {
          printf(
              "Failed to allocate memory for client/server/standalone mode\n");
          free_config(config); // Free memory allocated for Config struct
          return NULL;
        }
        strcpy(config->mode, (char *)event.data.scalar.value);
      } else if (strcmp((char *)event.data.scalar.value, "server_ip_addr") ==
                 0) {
        yaml_parser_parse(&parser, &event);
        config->server_ip_addr =
            malloc(strlen((char *)event.data.scalar.value) + 1);
        if (!config->server_ip_addr) {
          printf("Failed to allocate memory for server IP address\n");
          free_config(config); // Free memory allocated for Config struct
          return NULL;
        }
        strcpy(config->server_ip_addr, (char *)event.data.scalar.value);
      } else if (strcmp((char *)event.data.scalar.value, "src_port_udp") == 0) {
        yaml_parser_parse(&parser, &event);
        config->src_port_udp = atoi((char *)event.data.scalar.value);
      } else if (strcmp((char *)event.data.scalar.value, "dst_port_udp") == 0) {
        yaml_parser_parse(&parser, &event);
        config->dst_port_udp = atoi((char *)event.data.scalar.value);
      } else if (strcmp((char *)event.data.scalar.value, "dst_port_tcp_hsyn") ==
                 0) {
        yaml_parser_parse(&parser, &event);
        config->dst_port_tcp_hsyn = atoi((char *)event.data.scalar.value);
      } else if (strcmp((char *)event.data.scalar.value, "dst_port_tcp_tsyn") ==
                 0) {
        yaml_parser_parse(&parser, &event);
        config->dst_port_tcp_tsyn = atoi((char *)event.data.scalar.value);
      } else if (strcmp((char *)event.data.scalar.value, "pp_port_tcp") == 0) {
        yaml_parser_parse(&parser, &event);
        config->pp_port_tcp = atoi((char *)event.data.scalar.value);
      } else if (strcmp((char *)event.data.scalar.value, "payload_size") == 0) {
        yaml_parser_parse(&parser, &event);
        config->payload_size = atoi((char *)event.data.scalar.value);
      } else if (strcmp((char *)event.data.scalar.value, "inter_time_s") == 0) {
        yaml_parser_parse(&parser, &event);
        config->inter_time_s = atoi((char *)event.data.scalar.value);
      } else if (strcmp((char *)event.data.scalar.value, "udp_train_size") ==
                 0) {
        yaml_parser_parse(&parser, &event);
        config->udp_train_size = atoi((char *)event.data.scalar.value);
      } else if (strcmp((char *)event.data.scalar.value, "udp_ttl") == 0) {
        yaml_parser_parse(&parser, &event);
        config->udp_ttl = atoi((char *)event.data.scalar.value);
      } else if (strcmp((char *)event.data.scalar.value, "rst_timeout_s") ==
                 0) {
        yaml_parser_parse(&parser, &event);
        config->rst_timeout_s = atoi((char *)event.data.scalar.value);
      }

      break;
    case YAML_STREAM_END_EVENT:
      done = 1;
      break;
    default:
      break;
    }
  }
  return config;
}

// Free memory for Config struct
void free_config(struct Config *config) {
  free(config->mode);
  free(config->server_ip_addr);
  free(config);
}

// Prints config struct in a readable manner
void print_config(struct Config *config) {
  logger("");
  logger("YAML Config");
  logger("-----------");
  logger("mode: %s", config->mode);
  logger("server_ip_addr: %s", config->server_ip_addr);
  logger("src_port_udp: %d", config->src_port_udp);
  logger("dst_port_udp: %d", config->dst_port_udp);
  logger("dst_port_tcp_hsyn: %d", config->dst_port_tcp_hsyn);
  logger("dst_port_tcp_tsyn: %d", config->dst_port_tcp_tsyn);
  logger("pp_port_tcp: %d", config->pp_port_tcp);
  logger("payload_size: %d", config->payload_size);
  logger("inter_time_s: %d", config->inter_time_s);
  logger("udp_train_size: %d", config->udp_train_size);
  logger("udp_ttl: %d", config->udp_ttl);
  logger("rst_timeout_s: %d\n", config->rst_timeout_s);
}
