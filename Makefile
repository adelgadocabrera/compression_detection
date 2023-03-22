# Compiler settings
CC = gcc
CFLAGS = -Wall -Wextra -I include
LDFLAGS= -lyaml

# Directories
SRC_DIR = src
BIN_DIR = bin

# Source files
SRC_FILES = $(wildcard $(SRC_DIR)/*.c)
OBJ_FILES = $(patsubst $(SRC_DIR)/%.c, $(BIN_DIR)/%.o, $(SRC_FILES))

# Output file
O_FILE = compdetect

# User arguments
ARGS ?= config.yaml

# Build target
TARGET = $(BIN_DIR)/$(O_FILE)

all: $(TARGET)

$(TARGET): $(OBJ_FILES)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(BIN_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(BIN_DIR)/*.o $(TARGET)

run: 
	$(BIN_DIR)/$(O_FILE) $(ARGS)

run_client: 
	$(BIN_DIR)/$(O_FILE) ./configurations/client.yaml

run_client_verbose: 
	$(BIN_DIR)/$(O_FILE) ./configurations/client.yaml -v

run_server: 
	$(BIN_DIR)/$(O_FILE) ./configurations/server.yaml

run_server_verbose: 
	$(BIN_DIR)/$(O_FILE) ./configurations/server.yaml -v

run_standalone: 
	$(BIN_DIR)/$(O_FILE) ./configurations/standalone.yaml
