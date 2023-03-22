#include "../include/main.h"
#include "../include/client.h"
#include "../include/config.h"
#include "../include/logger.h"
#include "../include/server.h"
#include "../include/standalone.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

struct Args {
  char *filename;
  bool verbose;
};

struct Args *get_args(int argc, char *argv[]) {
  int opt;
  struct Args *args = malloc(sizeof(struct Args));
  args->verbose = false;
  args->filename = "";
  while ((opt = getopt(argc, argv, "vh")) != -1) {
    switch (opt) {
    case 'v':
      /* Handle -v (verbose) flag */
      debug_enabled = 1;
      logger("Running in verbose mode");
      break;
    case 'h':
      /* Handle -h (help) flag */
      printf("%s", HELP_MSG);
      exit(0);
    }
  }
  /* Handle non-flag arguments (if any) */
  args->filename = argv[optind];
  return args;
}

int main(int argc, char *argv[]) {
  struct Args *args = get_args(argc, argv);
  if (strcmp(args->filename, "") == 0) {
    printf("%s", RUN_PROGRAM_MSG);
    exit(1);
  }
  struct Config *config = malloc(sizeof(struct Config));
  init_config(config);
  if (parse_config(config, args->filename) == NULL) {
    printf("Something went wrong parsing YAML file.\n");
    exit(1);
  }
  if (debug_enabled) {
    print_config(config);
  }
  if (strcmp(config->mode, CLIENT_APP) == 0) {
    run_client(config);
  } else if (strcmp(config->mode, SERVER_APP) == 0) {
    run_server(config->pp_port_tcp);
  } else if (strcmp(config->mode, STANDALONE_APP) == 0) {
  }
  free_config(config);
}
