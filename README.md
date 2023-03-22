# Project 1 

## Project Structure 
`bin/`: This directory contains the executable file of the project, named my_project.

`configurations/`: In order to execute any of the programs (client/server/standalone) the program has to fetch the appropriate config.yaml. Config YAML files are located here and by default are called `client.yaml`, `server.yaml` and `standalone.yaml`.

`include/`: This directory contains the header files used by the project. It contains header1.h and header2.h, which define the interfaces of some functions used in the program.

`src/`: This directory contains the source code for the project. It contains a main.c file, which is the entry point of the program, and a util.c file, which contains some utility functions used by the program. The util.h header file defines the interface of these utility functions, and the Makefile is used to build the program.

`README.md`: This is a simple readme file that provides some basic information about the project.

## Installation 

### Dependencies 
Install `libyaml` from https://github.com/yaml/libyaml. Or you can follow these steps (taken from their README): 

    $ git clone https://github.com/yaml/libyaml && cd libyaml 
    $ ./configure
    $ make
    # make install

Required packages:

- gcc
- libtool
- make


### Compiling

Run `make` to compile.
