#define CLIENT_APP "client"         // name of client application
#define SERVER_APP "server"         // name of server application
#define STANDALONE_APP "standalone" // name of standalone application

// Message instructing the user to run the program
#define RUN_PROGRAM_MSG                                                        \
  "Please run the program by executing:\n$ compdetect <yaml_file>"

// Message explaining the usage of the program, including its options and
// arguments
#define HELP_MSG                                                               \
  "COMPDETECT - Detect network compression between a source and a "            \
  "destination\n\n"                                                            \
  "SYNOPSIS\n"                                                                 \
  "  compdetect <yaml_file> [-v] \n\n"                                         \
  "DESCRIPTION\n"                                                              \
  "  Compdetect is a program that detects network compression between a "      \
  "source and a\n"                                                             \
  "  destination. The program can run in two modes: Server/Client mode and "   \
  "Standalone\n"                                                               \
  "  mode. Server/Client mode is the more traditional and safer option, "      \
  "while Standalone\n"                                                         \
  "  mode requires some tricks to make it work.\n\n"                           \
  "OPTIONS\n"                                                                  \
  "  <yaml_file>\n"                                                            \
  "    The path to the YAML configuration file. This file specifies the "      \
  "components and\n"                                                           \
  "    their dependencies.\n\n"                                                \
  "  -v\n"                                                                     \
  "    Run the program in verbose mode. This will print log messages while "   \
  "running.\n\n"                                                               \
  "  -h\n"                                                                     \
  "    Display the help message.\n"
