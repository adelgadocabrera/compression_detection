#!/bin/bash

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
  docker stop $(docker ps -aqf "name=standalone-container") &> /dev/null
  docker rm $(docker ps -aqf "name=standalone-container") &> /dev/null
  docker rmi standalone &> /dev/null
  echo "The standalone image and container have been cleaned."
  exit 0
fi

# Build the standalone image
if [[ $build_image ]] || ! docker images standalone | awk '{print $2}' | grep -q 'latest'; then
    echo "The standalone image does not exist or is out of date. Building it now..."
    docker build -t standalone -f Dockerfile.standalone .
fi

# Run the standalone container
if [[ $run_container ]]; then
  docker run --rm -it --name standalone-container standalone make standalone 2>&1
fi

