#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "map.h"

// function to get the map size from the file
bool get_map_size(const char* filename, int* height, int* width)
{
    FILE* file = fopen(filename, "r");
    if (!file) {
        fprintf(stderr, "Could not open map file: %s\n", filename);
        return false;
    }

    char line[2048];
    *height = 0;
    *width = 0;

    while (fgets(line, sizeof(line), file)) {
        (*height)++;
        int len = strlen(line)-1; // -1 to remove the \n
            *width = len;
        
    }

    fclose(file);
    return true;
}

//function to allocate the map
bool allocate_map(enum tile_type_e*** map, int height, int width)
{
    // Allocate memory for the map
    if (!map || width <= 0 || height <= 0) return false;
    *map = malloc(height * sizeof(enum tile_type_e*));
    if (*map == NULL) {
        fprintf(stderr, "Could not allocate memory for map\n");
        return false;
    }

    for (int i = 0; i < height; i++) {
        (*map)[i] = malloc(width * sizeof(enum tile_type_e));
        if ((*map)[i] == NULL) {
            fprintf(stderr, "Could not allocate memory for map row %d\n", i);
            for(int j = 0; j < i; j++) {
                free((*map)[j]);
            }
            return false;
        }
    }

    return true;
}


//function to free the map
void free_map(enum tile_type_e** map, int height)
{
    for (int i = 0; i < height; i++) {
        free(map[i]);
    }
    free(map);
}


// function that loads the map from a file provided by the user
bool load_map_from_file(const char* filename, enum tile_type_e** map,  int* height  , int* width)
 {

    // Open the map file
    FILE* file = fopen(filename, "r");
    if (!file) {
        fprintf(stderr, "Could not open map file: %s\n", filename);
        free_map(map, *height);
        return false;
    }
   
    
    // Read the map file line by line
    char line[256];
    int y = 0;
    int ghost_count =0;
    while (fgets(line, sizeof(line), file) && y < *height) 
    {
        for (int x = 0; x < *width && line[x] != '\0' && line[x] != '\n'; x++) 
        {
            if (line[x] == 'W') map[y][x] = WALL;
            else if (line[x] == ' ') map[y][x] = PATH;
            else if (line[x] == 'S') map[y][x] = PACMAN_START;
            else if (line[x] == 'G' && ghost_count < 4) {  // Only allow up to 4 ghosts
                map[y][x] = GHOST_pink_START + ghost_count;
                ghost_count++;
            
            }
            else if (line[x] == 'G' && ghost_count >=4) {  // Only allow up to 4 ghosts
                map[y][x] = PATH;
                ghost_count++;
            
            }
            else {
                fprintf(stderr, "Error: weird character not (W, ,G,S)\n");
                free_map(map, *height);
                exit(EXIT_FAILURE);
            }
        }
        y++;
    }
    fclose(file);

    bool pacman_found = false;
    for (int y = 0; y < *height; y++) {
        for (int x = 0; x < *width; x++) {
            if (map[y][x] == PACMAN_START) {
                pacman_found = true;
            }
        }
    }
    if (!pacman_found) {
        fprintf(stderr, "Error: No Pacman start position (S) in map\n");
        free_map(map, *height);
        exit(EXIT_FAILURE);
    }
    return true;

 }

 //function that initializes the default map
void init_map(enum tile_type_e** map, int* height, int* width)
{
    *width = N_X_TILES;
    *height = N_Y_TILES;

     /* The map.
     * Not using WALL, PATH, ... to make it shorter */
    const enum tile_type_e default_map[N_Y_TILES][N_X_TILES] = {
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0},
        {0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0},
        {0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0},
        {0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0},
        {0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0},
        {0, 1, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 1, 0},
        {0, 1, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 1, 0},
        {0, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 0},
        {0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 1, 1, 1, 1, 1, 1, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 1, 1, 3, 4, 5, 6, 1, 1, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 1, 1, 1, 1, 1, 1, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0},
        {0, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 0},
        {0, 1, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 1, 0},
        {0, 1, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 1, 0},
        {0, 1, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 1, 0},
        {0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0},
        {0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0},
        {0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0},
        {0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    };
    if (!map || !width || !height) return;

    // Initialize the map with default values
    for (int y = 0; y < N_Y_TILES; y++) {
        for (int x = 0; x < N_X_TILES; x++) {
            map[y][x] = default_map[y][x];
        }
    }
}
