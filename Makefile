CC = gcc
CFLAGS = -g -Wall -Werror -std=c99 $(shell pkg-config --cflags sdl2 SDL2_image) 
LDFLAGS = $(shell pkg-config --libs sdl2 SDL2_image) 

all: pacman

pacman: pacman.c
	$(CC) $(CFLAGS) pacman.c -o pacman $(LDFLAGS)

clean: 
	rm -f pacman *.o

.PHONY: all clean install check 
