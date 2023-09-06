SRC=delta.c
BIN=delta
CC=gcc
FLAGS=-Wall -Wextra -pedantic -std=c99

build:
	$(CC) $(SRC) -o $(BIN) $(FLAGS)
