#ifndef MAP_H
#define MAP_H

#include <stdbool.h>

#define N_X_TILES 28
#define N_Y_TILES 29

#define PACGOMME_CYCLE 30
#define VULNERABLE_TIME 8000

enum tile_type_e {
    WALL = 0,
    PATH,
    PACMAN_START,
    GHOST_pink_START = 3,
    GHOST_red_START = 4,
    GHOST_blue_START = 5,
    GHOST_orange_START = 6
};

bool get_map_size(const char* filename, int* height, int* width);
bool allocate_map(enum tile_type_e*** map, int height, int width);
void free_map(enum tile_type_e** map, int height);
bool load_map_from_file(const char* filename, enum tile_type_e** map, int* height, int* width);
void init_map(enum tile_type_e** map, int* height, int* width);

#endif
