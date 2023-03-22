#!/bin/bash

# Build the C program
make

# Build the client Docker image
docker build -t myclientimage -f Dockerfile .

# Build the server Docker image
docker build -t myserverimage -f Dockerfile .
