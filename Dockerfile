# First stage: build the C program
FROM gcc AS build
WORKDIR /app
COPY . .
RUN make

# Second stage: create the client container
FROM debian:buster-slim AS client
WORKDIR /app
COPY --from=build /app /app
CMD ["make", "run_client"]

# Third stage: create the server container
FROM debian:buster-slim AS server
WORKDIR /app
COPY --from=build /app /app
RUN SERVER_PORT=$(grep "dst_port_udp" configurations/server.yaml | cut -d " " -f 2) && \
  sed -i "s/8080:$SERVER_PORT/$SERVER_PORT:$SERVER_PORT/g" Dockerfile && \
  EXPOSE $SERVER_PORT
CMD ["make", "run_server"]

