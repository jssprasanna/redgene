CC=/usr/bin/g++
CC_FLAGS=-std=c++11
EXE=redgene
SRC=redgene.cpp
INCLUDES=-I.

all:
	$(CC) $(CC_FLAGS) $(INCLUDES) -o $(EXE) $(SRC) 