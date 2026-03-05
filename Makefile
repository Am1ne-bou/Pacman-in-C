CC = gcc
CFLAGS = -g -Wall -Werror -std=c99 $(shell pkg-config --cflags sdl2 SDL2_image SDL2_ttf)
LDFLAGS = $(shell pkg-config --libs sdl2 SDL2_image SDL2_ttf)

SRC_DIR = src
SRCS = $(wildcard $(SRC_DIR)/*.c)

all: pacman

pacman: $(SRCS)
	$(CC) $(CFLAGS) $(SRCS) -o pacman $(LDFLAGS)

clean:
	rm -f pacman *.o

.PHONY: all clean