#!/bin/bash

# Read the server port from configurations/server.yaml
TCP_PORT=$(grep "pp_port_tcp" configurations/server.yaml | cut -d " " -f 2)
UDP_PORT=$(grep "dst_port_udp" configurations/client.yaml | cut -d " " -f 2)

# Set the SERVER_PORT environment variable for the server container
export TCP_PORT
export UDP_PORT

# Parse the command line arguments
while getopts "brc" arg; do
  case $arg in
    b) build_image=true;;
    r) run_container=true;;
    c) clean=true;;
    *) echo "Invalid argument: $OPTARG" >&2; exit 1;;
  esac
done

# Clean Docker images and containers
if [[ $clean ]]; then
  docker stop $(docker ps -aq) &> /dev/null
  docker rm $(docker ps -aq) &> /dev/null
  docker rmi $(docker images -f "dangling=true" -q) &> /dev/null
  docker rmi $(docker images --format "{{.Repository}}:{{.Tag}}" | grep -E '(server|client):[0-9]*' | tr '\n' ' ') &> /dev/null
  echo "Docker images and containers have been cleaned."
  exit 0
fi

# Check if the server_p1 and client_p1 images exist, otherwise build them
if docker images server_p1:latest | awk '{print $2}' | grep -q "latest" && \
   docker images client_p1:latest | awk '{print $2}' | grep -q "latest" && \
   [[ -z $build_image ]]; then
    echo "The server_p1 and client_p1 images are already up to date."
else
    echo "The server_p1 and/or client_p1 images do not exist or are out of date. Building them now..."
    docker-compose build --no-cache
fi

# Start the containers using Docker Compose
if [[ $run_container ]]; then
  docker-compose up
fi

