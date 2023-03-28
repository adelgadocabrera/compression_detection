# Project 1 
by Alberto Delgado Cabrera

This project provides a compression detection system with two main applications: client/server and standalone. This README file provides information on how to use the project and its features.

## Project Structure 
The project directory contains the following directories:
- `bin/`: This directory contains the executable file of the project, named `compdetect`. This path has been added to .gitignore to prevent from uploading bin files.
- `configurations/`: In order to execute any of the programs (client/server/standalone) the program has to fetch the appropriate config.yaml. Config YAML files are located here and by default are called `client.yaml`, `server.yaml` and `standalone.yaml`.
- `include/`: This directory contains the header files used by the project.
- `src/`: This directory contains the source code for the project. It contains a main.c file, which is the entry point of the program, and a util.c file, which contains some utility functions used by the program. The util.h header file defines the interface of these utility functions, and the Makefile is used to build the program.
- `README.md`: This is a simple readme file that provides some basic information about the project.
- `Dockerfile.XXXX`: Contains the necessary config options to create the corresponding images for client/server/standalone app 
- `part1.sh`: main script to run part 1 (client/server compression detection). More info down below
- `part2.sh`: main script to run part 2 (standalone compression detection). More info down below
- `Makefile`: compiles the program and provides execution shortcuts to run client/server/standalone programs

## Installation / Dependencies
To install the project dependencies and run the programs in isolated containers, the project uses Docker. To run both part 1 and part 2, you need to have:
- [Docker](https://docs.docker.com/engine/install/)
- [Docker Compose](https://pypi.org/project/docker-compose/) - `pip install docker-compose`


## How to run 
Provided are bash scripts to run the applications. Unfortunately, they need sudo permissions. (Part 2 uses raw sockets, and it is required to run Docker and Docker Compose with elevated privileges). There are two main programs: client/server and standalone. To run client/server, use part1.sh. To run the standalone application, use part2.sh. The following flags work for both:
- `-b` (re)builds Docker containers. Execute this when you want to update a new image in case you've made some changes. Otherwise, you can directly run -r.
- `-r` runs the program. If the image hasn't been built, it will build it first.
- `-c` cleans containers. Execute this when done.

For example, if you want to run part 1:
```bash
sudo ./part1.sh -b # for building (you can skip this step unless you want to force new build)
sudo ./part1.sh -r # for running
sudo ./part1.sh -c # for clean-up
```

Do the same for part 2 but use instead `part2.sh`.

### Running locally without containers
First install `libyaml`:
```bash
$ ./bootstrap
$ ./configure
$ make
$ make install
```

If you wish to test the client/server application or the standalone application the project provides a `Makefile` that will make it easy for you. 
- For either application first run `make`. 
- Client/server compression detection: run first `make server` or `make server_v` if you want to run in verbose mode. Immediately after run `make client` or `make client_v` to run in verbose mode. Client will wait a couple of seconds after executed just to make sure server is ready. Alternatively, you can directly run `make part1` and will run both server and client for you.
- Standalone compression detection: run `make standalone` or `make standalone_v` to run in verbose mode.
- Cleanup: Once you are done you may run `make clean` to delete any executable files in `bin` folder.

## PCAP files 
PCAP files may be found inside the `pcap` folder. The requirement was to run Wireshark at the sender in both cases, but because we are running both programs inside Docker containers, Wireshark is running can capturing from main computer. All packets have been captured properly, though.
