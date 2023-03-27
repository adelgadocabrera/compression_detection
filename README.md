# Project 1 

## Project Structure 
`bin/`: This directory contains the executable file of the project, named my_project.

`configurations/`: In order to execute any of the programs (client/server/standalone) the program has to fetch the appropriate config.yaml. Config YAML files are located here and by default are called `client.yaml`, `server.yaml` and `standalone.yaml`.

`include/`: This directory contains the header files used by the project. It contains header1.h and header2.h, which define the interfaces of some functions used in the program.

`src/`: This directory contains the source code for the project. It contains a main.c file, which is the entry point of the program, and a util.c file, which contains some utility functions used by the program. The util.h header file defines the interface of these utility functions, and the Makefile is used to build the program.

`README.md`: This is a simple readme file that provides some basic information about the project.

## Installation / Dependencies
Project uses docker to install dependencies and run programs in isolated containers. All you'll need to run both part1 and part2 is:
- [Docker](https://docs.docker.com/engine/install/)
- [Docker Compose](https://pypi.org/project/docker-compose/) - `pip install docker-compose`


## How to run 
Provided bash scripts to run the applications. Unfortunately they need sudo permissions (part 2 uses raw sockets and overall it is required to run docker and docker compose). There are 2 main programs: client/server and standalone. To run client/server you will use part1.sh and to run the standalone application you'll run part2.sh. The following flags work for either one:
- `-b` (re)builds docker containers. Execute this when you want to update a new image in case you've made some changes. Otherwise you can directly run `-r`.
- `-r` runs the program. If image hasn't been built it will build it first.
- `-c` cleans containers. Execute this when done.

So for example, if you want to run part 1:
```bash
sudo ./part1.sh -b # for building (you can skip this step)
sudo ./part1.sh -r # for running
sudo ./part1.sh -r # for clean-up
```

Do the same for part 2 but run instead `part2.sh`.
