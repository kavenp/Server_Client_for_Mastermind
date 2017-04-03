
# Makefile for Computer Systems Project 2
# Author: Kaven Peng kavenp 696573

## CC  = Compiler.
## CFLAGS = Compiler flags.
CC	= gcc
CFLAGS 	= -Wall


## OBJ = Object files.
## SRC = Source files.
## EXE = Executable name.

SRC1 =		client.c
OBJ1 =		client.o
EXE1 = 		client

SRC2 =		server.c
OBJ2 = 		server.o mastermind.o
EXE2 =      server

## Top level target is executable.
$(EXE1):	$(OBJ1)
		$(CC) $(CFLAGS) -o $(EXE1) $(OBJ1) -lm

$(EXE2):	$(OBJ2)
		$(CC) $(CFLAGS) -o $(EXE2) $(OBJ2) -lm -lpthread

## Clean: Remove object files and core dump files.
clean:
		/bin/rm $(OBJ1) $(OBJ2)

## Clobber: Performs Clean and removes executable file.

clobber: clean
		/bin/rm $(EXE1) $(EXE2)

## Dependencies

client.o: 
server.o: mastermind.h
mastermind.o: mastermind.h
