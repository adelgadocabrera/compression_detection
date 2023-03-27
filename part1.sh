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

# Check if the server and client images are up to date
version_file=".version"
if [[ -f $version_file ]]; then
  last_version=$(cat $version_file)
else
  last_version=1
fi
version=$((last_version))
NEW_VERSION=$((last_version + 1))
export NEW_VERSION

if docker images server:$version | awk '{print $2}' | grep -q "$version" && \
   docker images client:$version | awk '{print $2}' | grep -q "$version" && \
   [[ -z $build_image ]]; then
    echo "The server and client images are already up to date."
else
    echo "The server and/or client images do not exist or are out of date. Building them now..."
    docker-compose build --no-cache --build-arg VERSION=$NEW_VERSION
    docker image rm server:$NEW_VERSION client:$NEW_VERSION 
    echo $NEW_VERSION > $version_file
    sudo chmod 777 .version
fi

# Start the containers using Docker Compose
if [[ $run_container ]]; then
  docker-compose up
fi
