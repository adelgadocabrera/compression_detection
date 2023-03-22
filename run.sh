#!/bin/bash

# Read the server port from the configuration file
SERVER_PORT=$(grep "dst_port_udp" configurations/server.yaml | cut -d " " -f 2)

# Run the client Docker container
docker run --rm -it myclientimage

# Run the server Docker container with the specified port
docker run --rm -it -p $SERVER_PORT:$SERVER_PORT myserverimage

