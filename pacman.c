#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include <time.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

#define RENDER_SCALE 0.7

#define N_X_TILES 28
#define N_Y_TILES 29

#define PACMAN_SIZE 32

#define TILE_SIZE (PACMAN_SIZE+8)

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

struct Ghost
{
    SDL_Rect position;
    int direction;
    SDL_Texture* tex_up;
    SDL_Texture* tex_down;
    SDL_Texture* tex_left;
    SDL_Texture* tex_right;
    SDL_Texture** current_tex;
    int speed;
    int exists;
    int chase;

};


void set_window_position_coords(SDL_Rect* window_rect)
{
    // Find where to launch the window (at the center of the screen where is the cursor)
    int mouse_x, mouse_y;
    SDL_GetGlobalMouseState(&mouse_x, &mouse_y);

    int nb_displays = SDL_GetNumVideoDisplays();
    SDL_Rect display_bounds;
    for (int i = 0; i < nb_displays; i++)
    {
        SDL_GetDisplayBounds(i, &display_bounds);
        if (mouse_x >= display_bounds.x && mouse_x < (display_bounds.x + display_bounds.w) && mouse_y >= display_bounds.y && mouse_y <(display_bounds.y + display_bounds.h))
        {
            break;
        }
    }

    window_rect->x = display_bounds.x + (display_bounds.w - window_rect->w*RENDER_SCALE) / 2;
    window_rect->y = display_bounds.y + (display_bounds.h - window_rect->h*RENDER_SCALE) / 2;
}


// Convert pixel coordinates (x, y) to tile coordinates (tileX, tileY)
void pixelToTile(int pixelX, int pixelY, int *tileX, int *tileY) {
    *tileX = pixelX / TILE_SIZE;
    *tileY = pixelY / TILE_SIZE;
}

// Convert pixel coordinates (x, y) to tile coordinates (tileX, tileY)
void TileTopixel(int tileX,int tileY, int *pixelX,int *pixelY) {
    *pixelX=tileX*TILE_SIZE;
    *pixelY=tileY*TILE_SIZE;
}



// Check if a movement direction is valid 
bool can_move(int tileX, int tileY, int direction, enum tile_type_e** map,int height, int width) {
    int dx = 0, dy = 0;
    switch (direction) {
        case SDLK_UP:    dy = -1; break;
        case SDLK_DOWN:  dy =  1; break;
        case SDLK_LEFT:  dx = -1; break;
        case SDLK_RIGHT: dx =  1; break;
    }
    int nextX = tileX + dx;
    int nextY = tileY + dy;
    if (nextX < 0 || nextX >= width || nextY < 0 || nextY >= height) {
        return false;
    }
    return (map[nextY][nextX] != WALL);  
}

// Updates the entity's texture based on direction
void set_entity_texture(SDL_Texture** current, int direction, 
                        SDL_Texture* up, SDL_Texture* down,
                        SDL_Texture* left, SDL_Texture* right) {
    switch (direction) {
        case SDLK_UP:    *current = up;    break;
        case SDLK_DOWN:  *current = down;  break;
        case SDLK_LEFT:  *current = left;  break;
        case SDLK_RIGHT: *current = right; break;
    }
}

// Function to move the entity (Pacman or Ghost)
void move_entity(SDL_Rect* entity, int* move_dir, int* requested_dir,int* speed, 
                enum tile_type_e** map, int height, int width,
                SDL_Texture** current_texture,
                SDL_Texture* tex_up, SDL_Texture* tex_down,
                SDL_Texture* tex_left, SDL_Texture* tex_right) 
{
    //  Check if entity is at the center of a tile 
    //  and convert to tile coordinates 
    int centerX = entity->x + PACMAN_SIZE / 2;
    int centerY = entity->y + PACMAN_SIZE / 2;
    int gridX, gridY;
    pixelToTile(centerX, centerY, &gridX, &gridY);
    int tileCenterX = gridX * TILE_SIZE + TILE_SIZE / 2;
    int tileCenterY = gridY * TILE_SIZE + TILE_SIZE / 2;

    // Check if requested direction is valid and chande move_dir if its the case
    if (centerX == tileCenterX && centerY == tileCenterY) {
        if (can_move(gridX, gridY, *requested_dir, map,height,width)) {
            *move_dir = *requested_dir;
        }
    }

    //  Movement 
    int nextX = entity->x;
    int nextY = entity->y;
    switch (*move_dir) {
        case SDLK_UP:
            nextY -= *speed;
            pixelToTile(nextX, nextY - (TILE_SIZE-PACMAN_SIZE)/2, &gridX, &gridY);
            if (map[gridY][gridX] != WALL) entity->y = nextY;
            break;
                        
        case SDLK_DOWN:
            nextY += *speed;
            pixelToTile(nextX, nextY + PACMAN_SIZE + (TILE_SIZE-PACMAN_SIZE)/2 - 1, &gridX, &gridY);
            if (map[gridY][gridX] != WALL) entity->y = nextY;
            break;
         
        case SDLK_LEFT:
            nextX -= *speed;
            pixelToTile(nextX - (TILE_SIZE-PACMAN_SIZE)/2, nextY, &gridX, &gridY);
            if (map[gridY][gridX] != WALL) entity->x = nextX;
            break;
             
        case SDLK_RIGHT:
            nextX += *speed;
            pixelToTile(nextX + PACMAN_SIZE + (TILE_SIZE-PACMAN_SIZE)/2 - 1, nextY, &gridX, &gridY);
            if (map[gridY][gridX] != WALL) entity->x = nextX;
            break;
    }

    //  Texture update 
    set_entity_texture(current_texture, *move_dir, tex_up, tex_down, tex_left, tex_right);
}

// Function to calculate the distance between two entities
int distance_between_two_entities(SDL_Rect* entity1, SDL_Rect* entity2) {
    int centerX1 = entity1->x + PACMAN_SIZE / 2;
    int centerY1 = entity1->y + PACMAN_SIZE / 2;
    int centerX2 = entity2->x + PACMAN_SIZE / 2;
    int centerY2 = entity2->y + PACMAN_SIZE / 2;

    return (centerX2 - centerX1) * (centerX2 - centerX1) + (centerY2 - centerY1) * (centerY2 - centerY1);

}

// Function to get the ghost direction randomly
int get_ghost_direction_randomly(SDL_Rect* ghost_pos, int current_dir, enum tile_type_e** map, int height, int width)     
{       
         // Check if ghost is centered on a tile 
        int centerX = ghost_pos->x + PACMAN_SIZE / 2;
        int centerY = ghost_pos->y + PACMAN_SIZE / 2;
        int tileX, tileY;
        pixelToTile(centerX, centerY, &tileX, &tileY);
        
        int tileCenterX = tileX * TILE_SIZE + TILE_SIZE / 2;
        int tileCenterY = tileY * TILE_SIZE + TILE_SIZE / 2;
        
        // Only consider direction changes when perfectly centered
        if (centerX != tileCenterX || centerY != tileCenterY) {
            return current_dir; 
        }
        int directions[] = {SDLK_UP, SDLK_DOWN, SDLK_LEFT, SDLK_RIGHT};
        int opposite_dir;

        // Determine forbidden opposite direction
        switch(current_dir) {
            case SDLK_UP: opposite_dir = SDLK_DOWN; break;
            case SDLK_DOWN: opposite_dir = SDLK_UP; break;
            case SDLK_LEFT: opposite_dir = SDLK_RIGHT; break;
            case SDLK_RIGHT: opposite_dir = SDLK_LEFT; break;
            default: opposite_dir = -1; break; 
        }

        // Try directions in random order
        for (int i = 0; i < 4; i++) 
        {
            int test_dir = directions[rand() % 4];

            // Skip opposite direction
            if (test_dir == opposite_dir) continue;

            int tileX, tileY;
            pixelToTile(ghost_pos->x + PACMAN_SIZE/2, ghost_pos->y + PACMAN_SIZE/2,&tileX, &tileY);


            if (can_move(tileX, tileY, test_dir, map, height, width)) {
                return test_dir;
            }
        }

        // If all else fails, continue current direction
        return current_dir;
}

// Function to get the ghost direction when chasing Pacman
int get_ghost_direction_chase(SDL_Rect* ghost_pos, SDL_Rect* pacman_pos,int pacman_current_dir, int current_dir, enum tile_type_e** map, int height, int width, int* mode_chase)     
{       
         // Check if ghost is centered on a tile 
        int centerX = ghost_pos->x + PACMAN_SIZE / 2;
        int centerY = ghost_pos->y + PACMAN_SIZE / 2;
        int tileX, tileY;
        pixelToTile(centerX, centerY, &tileX, &tileY);
        
        int tileCenterX = tileX * TILE_SIZE + TILE_SIZE / 2;
        int tileCenterY = tileY * TILE_SIZE + TILE_SIZE / 2;
        
        // Only consider direction changes when perfectly centered
        if (centerX != tileCenterX || centerY != tileCenterY) {
            return current_dir; 
        }

        int directions[] = {SDLK_UP, SDLK_DOWN, SDLK_LEFT, SDLK_RIGHT};
        int opposite_dir;
        int dx = 0, dy = 0;

        // Determine forbidden opposite direction
        switch(current_dir) {
            case SDLK_UP: opposite_dir = SDLK_DOWN;   break;
            case SDLK_DOWN: opposite_dir = SDLK_UP;  break;
            case SDLK_LEFT: opposite_dir = SDLK_RIGHT;  break;
            case SDLK_RIGHT: opposite_dir = SDLK_LEFT;  break;
            default: opposite_dir = -1; break; 
        }

        switch(pacman_current_dir) {
            case SDLK_UP: dy=-4*TILE_SIZE;   break;
            case SDLK_DOWN: dy=4*TILE_SIZE;  break;
            case SDLK_LEFT: dx=-4*TILE_SIZE;  break;
            case SDLK_RIGHT: dx=4*TILE_SIZE;  break;
            default: dx=0; dy=0; break;
        }

        SDL_Rect pacman_pos_copy = *pacman_pos;
        pacman_pos_copy.x = pacman_pos->x + dx;
        pacman_pos_copy.y = pacman_pos->y + dy;

        int best_dir = current_dir;
        int min_dist = 10000000; // Initialize to a large value
        
        for (int i = 0; i < 4; i++) 
        {
            int test_dir = directions[i];

            // Skip opposite direction
            if (test_dir == opposite_dir) continue;

            int tileX, tileY;
            pixelToTile(ghost_pos->x + PACMAN_SIZE/2, ghost_pos->y + PACMAN_SIZE/2,&tileX, &tileY);

            if (can_move(tileX, tileY, test_dir, map, height, width)) {

                SDL_Rect next_ghost_pos=*ghost_pos;

                switch (test_dir) {
                    case SDLK_UP: next_ghost_pos.y-=TILE_SIZE; break;
                    case SDLK_DOWN: next_ghost_pos.y+=TILE_SIZE; break;
                    case SDLK_LEFT: next_ghost_pos.x-=TILE_SIZE; break;
                    case SDLK_RIGHT: next_ghost_pos.x+=TILE_SIZE; break;
                }

                // Calculate distance to Pacman
                int distance = 0;
                if(*mode_chase==1)
                {
                    distance = distance_between_two_entities(&next_ghost_pos, pacman_pos);
                }
                if (*mode_chase==2)
                {
                    distance = distance_between_two_entities(&next_ghost_pos, &pacman_pos_copy);
                }
                                                
                if (distance < min_dist) 
                {
                    min_dist = distance;
                    best_dir = test_dir;
                }
                

            }
        }


        return best_dir;
}

// Function to that get the direction of the ghost when it is vulnerable to flee
int get_ghost_direction_escape(SDL_Rect* ghost_pos, SDL_Rect* pacman_pos, enum tile_type_e** map, int height, int width, int current_dir)
{
             // Check if ghost is centered on a tile 
        int centerX = ghost_pos->x + PACMAN_SIZE / 2;
        int centerY = ghost_pos->y + PACMAN_SIZE / 2;
        int tileX, tileY;
        pixelToTile(centerX, centerY, &tileX, &tileY);
        
        int tileCenterX = tileX * TILE_SIZE + TILE_SIZE / 2;
        int tileCenterY = tileY * TILE_SIZE + TILE_SIZE / 2;
        
        // Only consider direction changes when perfectly centered
        if (centerX != tileCenterX || centerY != tileCenterY) {
            return current_dir; 
        }

        int directions[] = {SDLK_UP, SDLK_DOWN, SDLK_LEFT, SDLK_RIGHT};
        int opposite_dir;


        // Determine forbidden opposite direction
        switch(current_dir) {
            case SDLK_UP: opposite_dir = SDLK_DOWN;   break;
            case SDLK_DOWN: opposite_dir = SDLK_UP;  break;
            case SDLK_LEFT: opposite_dir = SDLK_RIGHT;  break;
            case SDLK_RIGHT: opposite_dir = SDLK_LEFT;  break;
            default: opposite_dir = -1; break; 
        }


        int best_dir = current_dir;
        int max_dist = -10;
        
        for (int i = 0; i < 4; i++) 
        {
            int test_dir = directions[i];

            // Skip opposite direction
            if (test_dir == opposite_dir) continue;

            int tileX, tileY;
            pixelToTile(ghost_pos->x + PACMAN_SIZE/2, ghost_pos->y + PACMAN_SIZE/2,&tileX, &tileY);

            if (can_move(tileX, tileY, test_dir, map, height, width)) {

                SDL_Rect next_ghost_pos=*ghost_pos;

                switch (test_dir) {
                    case SDLK_UP: next_ghost_pos.y-=TILE_SIZE; break;
                    case SDLK_DOWN: next_ghost_pos.y+=TILE_SIZE; break;
                    case SDLK_LEFT: next_ghost_pos.x-=TILE_SIZE; break;
                    case SDLK_RIGHT: next_ghost_pos.x+=TILE_SIZE; break;
                }

                // Calculate distance to Pacman
                int distance = 0;
                distance = distance_between_two_entities(&next_ghost_pos, pacman_pos);
                                                
                if (distance > max_dist) 
                {
                    max_dist = distance;
                    best_dir = test_dir;
                }
                

            }
        }


        return best_dir;

}


// Function to update the ghost position and check for collisions
void updating_ghost(struct Ghost* ghost, SDL_Texture* vulnerable_tex, SDL_Renderer* renderer, SDL_Rect* pacman_pos,int* pacman_current_dir ,
    enum tile_type_e** map,int height,int width, int* vulnerable, int* game_on)
{
    if(!(ghost->exists))
    {
        ghost->position.x = -100;
        ghost->position.y = -100;
        return;
    }

    
    // Get the ghosts direction
    if(*vulnerable==1)
    {
        ghost->direction = get_ghost_direction_escape(&ghost->position, pacman_pos, map, height, width, ghost->direction);
    }
    else
    {
        if(ghost->chase==0)
        {
            ghost->direction = get_ghost_direction_randomly(&ghost->position, ghost->direction, map, height, width);  
        }
        else
        {
            ghost->direction = get_ghost_direction_chase(&ghost->position, pacman_pos, *pacman_current_dir, ghost->direction, map,
                height, width, &ghost->chase);
        }
    }
    // Move the ghost 
    move_entity(&ghost->position, &ghost->direction, &ghost->direction, &ghost->speed, map,height,width, ghost->current_tex,
                *vulnerable==1 ? vulnerable_tex : ghost->tex_up, *vulnerable==1 ? vulnerable_tex : ghost->tex_down,
                *vulnerable==1 ? vulnerable_tex : ghost->tex_left, *vulnerable==1 ? vulnerable_tex : ghost->tex_right);
    
    //show the ghost
    SDL_RenderCopy(renderer, *ghost->current_tex, NULL, &ghost->position);
    
    // Check if the ghost is on pacman and if the ghost vulnerability
    if (SDL_HasIntersection(&ghost->position, pacman_pos)) {
        if (*vulnerable == 1)
            ghost->exists = 0;
        else
            *game_on = 0;
    }

}

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
        {0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0},
        {0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0},
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


int main(int argc, char* argv[])
{
    //random seed
    srand(time(NULL)); 
    // Load the map
    enum tile_type_e** map=NULL;
    int map_width, map_height;
    if (argc > 1) {
        // Get the map size
        if (!get_map_size(argv[1], &map_height, &map_width)||
            !allocate_map(&map, map_height, map_width)||
            !load_map_from_file(argv[1], map, &map_height, &map_width)) 
        {
            // If the map file is not valid, use the default map
            fprintf(stderr, "Could not get map \n");
            map_height = N_Y_TILES;
            map_width = N_X_TILES;
            // Allocate memory for the map
            if (!allocate_map(&map, map_height, map_width)) {
                fprintf(stderr, "Could not allocate memory for map\n");

            }
            // Initialize the default map
            init_map(map, &map_height, &map_width);
            fprintf(stderr, "Using default map\n");

        }
    } else {
        // Use the default map size
        map_height = N_Y_TILES;
        map_width = N_X_TILES;
        // Allocate memory for the map
        if (!allocate_map(&map, map_height, map_width)) {
            fprintf(stderr, "Could not allocate memory for map\n");

        }
        // Initialize the default map
        init_map(map, &map_height, &map_width);
    }

   

    int ret;
    int is_running = 1;
    const int tile_size = PACMAN_SIZE+8;
    // Set the window size 
    SDL_Rect window_rect = {
        .w = map_width * tile_size,
        .h = map_height * tile_size,
        .x = SDL_WINDOWPOS_UNDEFINED,
        .y = SDL_WINDOWPOS_UNDEFINED,
    };

        // initialisation directions
        int move_direction = 0;
        int requested_direction =0;     
    
    
        bool pacman_open = true;             // état : bouche ouverte (true) ou fermée (false)
        Uint32 last_toggle_time = SDL_GetTicks(); // en millisecondes
        const Uint32 toggle_delay = 150;     // délai d'alternance en ms (ajustable)
    
        // initialisation of the pacgommene eaten
        int nb_pacgomme_eaten=0;
        int cycle_pacgomme=0;
    
        Uint32 vulnerable_start_time = 0;
    
        int speed_pacman = 1;
        int speed_ghost = 1;
    
        int vulnerable =0;
    
        int game_on = 1;

    // SDL initialisation with video support
    ret = SDL_Init(SDL_INIT_VIDEO);
    if (ret != 0)
    {
        fprintf(stderr, "Could not init SDL: %s\n", SDL_GetError());
        // Free the map
        free_map(map, map_height);
        return EXIT_FAILURE;
    }

    // Create the window
    set_window_position_coords(&window_rect);
    SDL_Window *screen = SDL_CreateWindow(
        "Pacman",
        window_rect.x, window_rect.y,
        window_rect.w*RENDER_SCALE, window_rect.h*RENDER_SCALE,
        0
    );
    if (!screen)
    {
        fprintf(stderr, "Could not create SDL screen: %s\n", SDL_GetError());
      // Free the map
         free_map(map, map_height);
        return EXIT_FAILURE;
    }

    // Create the renderer, can be seen as a paint brush
    SDL_Renderer *renderer = SDL_CreateRenderer(screen, -1, SDL_RENDERER_SOFTWARE);
    if (!renderer)
    {
        fprintf(stderr, "Could not create SDL renderer: %s\n", SDL_GetError());
        // Free the map
        free_map(map, map_height);
        return EXIT_FAILURE;
    }

    // Will apply a scale of RENDER_SCALE to all coordinates and dimensions handled by renderer
    ret = SDL_RenderSetScale(renderer, RENDER_SCALE, RENDER_SCALE);
    if (ret < 0)
    {
        fprintf(stderr, "Could not scale SDL renderer: %s\n", SDL_GetError());
           // Free the map
           free_map(map, map_height);
        return EXIT_FAILURE;
    }


    // Load textures
     

       // Load the image as a texture
       SDL_Texture* pacman_texture_left = IMG_LoadTexture(renderer, "images/pacman-left.png");
       SDL_Texture* pacman_texture_right = IMG_LoadTexture(renderer, "images/pacman-right.png");
       SDL_Texture* pacman_texture_up = IMG_LoadTexture(renderer, "images/pacman-up.png");
       SDL_Texture* pacman_texture_down = IMG_LoadTexture(renderer, "images/pacman-down.png");
       SDL_Texture* pacman_texture_full = IMG_LoadTexture(renderer, "images/pacman-full.png");
   
       SDL_Texture* ghost_texture_pink_up = IMG_LoadTexture(renderer, "images/ghost-pink-up.png");
       SDL_Texture* ghost_texture_pink_down = IMG_LoadTexture(renderer, "images/ghost-pink-down.png");
       SDL_Texture* ghost_texture_pink_left = IMG_LoadTexture(renderer, "images/ghost-pink-left.png");
       SDL_Texture* ghost_texture_pink_right = IMG_LoadTexture(renderer, "images/ghost-pink-right.png");
   
       SDL_Texture* ghost_texture_red_up = IMG_LoadTexture(renderer, "images/ghost-red-up.png");
       SDL_Texture* ghost_texture_red_down = IMG_LoadTexture(renderer, "images/ghost-red-down.png");
       SDL_Texture* ghost_texture_red_left = IMG_LoadTexture(renderer, "images/ghost-red-left.png");
       SDL_Texture* ghost_texture_red_right = IMG_LoadTexture(renderer, "images/ghost-red-right.png");
   
       SDL_Texture* ghost_texture_blue_up = IMG_LoadTexture(renderer, "images/ghost-blue-up.png");
       SDL_Texture* ghost_texture_blue_down = IMG_LoadTexture(renderer, "images/ghost-blue-down.png");
       SDL_Texture* ghost_texture_blue_left = IMG_LoadTexture(renderer, "images/ghost-blue-left.png");
       SDL_Texture* ghost_texture_blue_right = IMG_LoadTexture(renderer, "images/ghost-blue-right.png");
   
       SDL_Texture* ghost_texture_orange_up = IMG_LoadTexture(renderer, "images/ghost-orange-up.png");
       SDL_Texture* ghost_texture_orange_down = IMG_LoadTexture(renderer, "images/ghost-orange-down.png");
       SDL_Texture* ghost_texture_orange_left = IMG_LoadTexture(renderer, "images/ghost-orange-left.png");
       SDL_Texture* ghost_texture_orange_right = IMG_LoadTexture(renderer, "images/ghost-orange-right.png");
   
       SDL_Texture* ghost_texture_vulnerable = IMG_LoadTexture(renderer, "images/ghost-white-dizzy.png");
   
   
       if (!pacman_texture_left||!pacman_texture_right || !pacman_texture_up || !pacman_texture_down || !pacman_texture_full || 
           !ghost_texture_pink_up || !ghost_texture_pink_down || !ghost_texture_pink_left || !ghost_texture_pink_right
           || !ghost_texture_red_up || !ghost_texture_red_down ||!ghost_texture_red_left ||!ghost_texture_red_right
           || !ghost_texture_orange_up || !ghost_texture_orange_down ||!ghost_texture_orange_left ||!ghost_texture_orange_right
           || !ghost_texture_blue_up || !ghost_texture_blue_down ||!ghost_texture_blue_left ||!ghost_texture_blue_right||!ghost_texture_vulnerable)
       {
           fprintf(stderr, "Could not load image: %s\n", SDL_GetError());
           // Free the map
           free_map(map, map_height);
           return EXIT_FAILURE;
       }

    SDL_Texture* current_pacman_texture = pacman_texture_right;
    SDL_Texture* current_pink_ghost_texture = ghost_texture_pink_up;
    SDL_Texture* current_red_ghost_texture = ghost_texture_red_up;
    SDL_Texture* current_blue_ghost_texture = ghost_texture_blue_up;
    SDL_Texture* current_orange_ghost_texture = ghost_texture_orange_up;
       

    SDL_Rect pacman_position = {
        .w = PACMAN_SIZE,
        .h = PACMAN_SIZE,
    };

    SDL_Rect map_position = {
        .x = 0,
        .y = 0,
        .w = window_rect.w,
        .h = window_rect.h,
    };

    int taillePacgomme = TILE_SIZE / 3;
    SDL_Rect posgomme={
        .w = taillePacgomme,
        .h = taillePacgomme,
    };
 
    // creating tab_pos_gommes
    int tab_pos_gommes[map_height][map_width];
    
    for (int i =0; i < map_height; i++){
        for(int j = 0; j < map_width; j++)
        {
            if(map[i][j]==WALL)
            {
                tab_pos_gommes[i][j]=0;
            }
            else
            {
                tab_pos_gommes[i][j]=1;
            }
        }
    }

    //creating the ghosts

    struct Ghost pink_ghost = {
        .position = {.w = PACMAN_SIZE, .h = PACMAN_SIZE},
        .direction = SDLK_LEFT,
        .tex_up = ghost_texture_pink_up,
        .tex_down = ghost_texture_pink_down,
        .tex_left = ghost_texture_pink_left,
        .tex_right = ghost_texture_pink_right,
        .current_tex = &current_pink_ghost_texture,
        .speed = speed_ghost,
        .exists = 1,
        .chase = 2,

    };
    struct Ghost red_ghost = {
        .position = {.w = PACMAN_SIZE, .h = PACMAN_SIZE},
        .direction = SDLK_LEFT,
        .tex_up = ghost_texture_red_up,
        .tex_down = ghost_texture_red_down,
        .tex_left = ghost_texture_red_left,
        .tex_right = ghost_texture_red_right,
        .current_tex = &current_red_ghost_texture,
        .speed = speed_ghost,
        .exists = 1,
        .chase = 1,

    };
    struct Ghost blue_ghost = {
        .position = {.w = PACMAN_SIZE, .h = PACMAN_SIZE},
        .direction = SDLK_LEFT,
        .tex_up = ghost_texture_blue_up,
        .tex_down = ghost_texture_blue_down,
        .tex_left = ghost_texture_blue_left,
        .tex_right = ghost_texture_blue_right,
        .current_tex = &current_blue_ghost_texture,
        .speed = speed_ghost,
        .exists = 1,
        .chase = 0 ,

    };
    struct Ghost orange_ghost = {
        .position = {.w = PACMAN_SIZE, .h = PACMAN_SIZE},
        .direction = SDLK_LEFT,
        .tex_up = ghost_texture_orange_up,
        .tex_down = ghost_texture_orange_down,
        .tex_left = ghost_texture_orange_left,
        .tex_right = ghost_texture_orange_right,
        .current_tex = &current_orange_ghost_texture,
        .speed = speed_ghost,
        .exists = 1,
        .chase = 0,

    };

    struct Ghost ghosts[4] = { pink_ghost, red_ghost, blue_ghost, orange_ghost };

    // Create a texture for the map, can be seen as a layer
    SDL_Texture* map_texture = SDL_CreateTexture(
        renderer,
        SDL_PIXELFORMAT_RGBA8888,
        SDL_TEXTUREACCESS_TARGET,
        window_rect.w,
        window_rect.h
    );

    // Now, the paint brush `renderer` will paint on the layer `map_texture`
    SDL_SetRenderTarget(renderer, map_texture);

    // Select color of the paint brush (R, G, B, alpha)
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    // Paint the the whole target of the paint brush (`map_texture` now)
    SDL_RenderClear(renderer);

    // Now, let's paint the maze walls in blue:
    SDL_SetRenderDrawColor(renderer, 192, 188, 185, 255);
    for (int y = 0; y < map_height; y++)
    {
        for (int x = 0; x < map_width; x++)
        {
            if (map[y][x] == WALL)
            {
                SDL_Rect rectangle = {
                    .x = x*tile_size,
                    .y = y*tile_size,
                    .w = tile_size,
                    .h = tile_size,
                };
                SDL_RenderFillRect(renderer, &rectangle);
            }
            else if (map[y][x] == PACMAN_START)
            {
                // While we are at it, save what is the initial position of Pac-Man
                pacman_position.x = x*tile_size + (tile_size-PACMAN_SIZE)/2;
                pacman_position.y = y*tile_size + (tile_size-PACMAN_SIZE)/2;

                /* Don't forget to change the map tile type to consider it as a
                 * path (can be done probably in a better way, for instance
                 * with a bit fields) */
                
            }
            else if (map[y][x] == GHOST_pink_START)
            {
                ghosts[0].position.x = x*tile_size + (tile_size-PACMAN_SIZE)/2;
                ghosts[0].position.y = y*tile_size + (tile_size-PACMAN_SIZE)/2;
                
            }
            else if (map[y][x] == GHOST_red_START)
            {
                ghosts[1].position.x = x*tile_size + (tile_size-PACMAN_SIZE)/2;
                ghosts[1].position.y = y*tile_size + (tile_size-PACMAN_SIZE)/2;
                
            }
            else if (map[y][x] == GHOST_blue_START)
            {
                ghosts[2].position.x = x*tile_size + (tile_size-PACMAN_SIZE)/2;
                ghosts[2].position.y = y*tile_size + (tile_size-PACMAN_SIZE)/2;
                
            }
            else if (map[y][x] == GHOST_orange_START)
            {
                ghosts[3].position.x = x*tile_size + (tile_size-PACMAN_SIZE)/2;
                ghosts[3].position.y = y*tile_size + (tile_size-PACMAN_SIZE)/2;
                
            }         
        }
    }
   
   // Draw back to window's renderer (ie the paint brush draws on the window now):
   SDL_SetRenderTarget(renderer, NULL);

    while (is_running)
    {
        // Fetch event
        SDL_Event event;
        SDL_PollEvent(&event);

        switch (event.type)
                {
                    case SDL_QUIT:
                        is_running = 0;
                        break;
                }

        if(game_on==1)
        {

            // Clear the window: remove everything that was drawn
            SDL_RenderClear(renderer);
            // Draw the map texture in the window
            SDL_RenderCopy(renderer, map_texture, NULL, &map_position);

            switch (event.type)
            {
                case SDL_QUIT:
                    is_running = 0;
                    break;

                case SDL_KEYDOWN:
                    // A keyboard key was pressed down
                    switch (event.key.keysym.sym)
                    {
                        // It was a `q`, quit the program by exiting this loop
                        case SDLK_q:
                            is_running = 0;
                            break;    
                        case SDLK_UP:
                            requested_direction = event.key.keysym.sym;
                            break;
                        case SDLK_DOWN:
                            requested_direction = event.key.keysym.sym;
                            break;
                        case SDLK_LEFT:
                            requested_direction = event.key.keysym.sym;
                            break;
                        case SDLK_RIGHT:
                            requested_direction = event.key.keysym.sym;
                            break;
                    }
                    break;
            }

            // Move pacman 
            move_entity(&pacman_position, &move_direction, &requested_direction,&speed_pacman, map, map_height, map_width,
                &current_pacman_texture,
                pacman_texture_up, pacman_texture_down,
                pacman_texture_left, pacman_texture_right);

            // Draw the pacman texture in the window

                Uint32 current_time = SDL_GetTicks();
                if (current_time - last_toggle_time >= toggle_delay) {
                    pacman_open = !pacman_open;      // inverse l'état
                    last_toggle_time = current_time;
                }

                SDL_Texture* texture_to_render = pacman_open ? current_pacman_texture : pacman_texture_full;
                    
    
                
                SDL_RenderCopy(renderer, texture_to_render, NULL, &pacman_position);


            //Updating the ghosts

            for (int i = 0; i < 4; i++)
            {
                updating_ghost(&ghosts[i], ghost_texture_vulnerable, renderer, &pacman_position, &move_direction, map, map_height, map_width, &vulnerable, &game_on);
            }
             

            // EAT
            int pos_tile_pacman_x;
            int pos_tile_pacman_y;
            pixelToTile(pacman_position.x,pacman_position.y,&pos_tile_pacman_x,&pos_tile_pacman_y);
                if (tab_pos_gommes[pos_tile_pacman_y][pos_tile_pacman_x] == 1) {
                    tab_pos_gommes[pos_tile_pacman_y][pos_tile_pacman_x] = 0; 
                    nb_pacgomme_eaten++;
                    cycle_pacgomme++;
                }


            if (cycle_pacgomme > PACGOMME_CYCLE && vulnerable==0) {
                vulnerable_start_time = SDL_GetTicks();  // Enregistre l'heure actuelle
                vulnerable=1;
            }
            
            // Vérifie si 5 secondes se sont écoulées
            if (vulnerable==1 && (SDL_GetTicks() - vulnerable_start_time >= VULNERABLE_TIME)) {
                // Les fantômes redeviennent normaux après 5 secondes        
                vulnerable=0;
                vulnerable_start_time=0;
                cycle_pacgomme=0;

            }

            // Draw the pacgomme texture
            for (int i = 0; i < map_height; i++) 
            {
                for (int j = 0; j < map_width; j++) {
                    if (tab_pos_gommes[i][j] == 1) {
    
                        posgomme.x = j * tile_size + (tile_size - posgomme.w) / 2;
                        posgomme.y = i * tile_size + (tile_size - posgomme.h) / 2;
                
                       SDL_RenderCopy(renderer, pacman_texture_full, NULL, &posgomme);
                    }
                }
            }
                // Really show on the screen what we drew so far
                SDL_RenderPresent(renderer);

                // Wait 2 ms (reduce a bit processor usage and regulates speed of Pac-Man)
                SDL_Delay(2);
        }
        else
        {
            // Game over screen
            SDL_RenderClear(renderer);
            SDL_RenderCopy(renderer, map_texture, NULL, &map_position);
            SDL_RenderCopy(renderer, current_pacman_texture, NULL, &pacman_position);
            SDL_RenderCopy(renderer, *ghosts[0].current_tex, NULL, &ghosts[0].position);
            SDL_RenderCopy(renderer, *ghosts[1].current_tex, NULL, &ghosts[1].position);
            SDL_RenderCopy(renderer, *ghosts[2].current_tex, NULL, &ghosts[2].position);
            SDL_RenderCopy(renderer, *ghosts[3].current_tex, NULL, &ghosts[3].position);
            
            SDL_RenderPresent(renderer);
            SDL_Delay(2);

        }
    }
    // Free the map
    free_map(map, map_height);

    // Free all created resources
    SDL_DestroyTexture(pacman_texture_right);
    SDL_DestroyTexture(pacman_texture_up);
    SDL_DestroyTexture(pacman_texture_down);
    SDL_DestroyTexture(pacman_texture_left);
    SDL_DestroyTexture(pacman_texture_full);

    SDL_DestroyTexture(ghost_texture_blue_down);
    SDL_DestroyTexture(ghost_texture_blue_left);
    SDL_DestroyTexture(ghost_texture_blue_right);
    SDL_DestroyTexture(ghost_texture_blue_up);

    SDL_DestroyTexture(ghost_texture_orange_down);
    SDL_DestroyTexture(ghost_texture_orange_up);
    SDL_DestroyTexture(ghost_texture_orange_left);
    SDL_DestroyTexture(ghost_texture_orange_right);

    SDL_DestroyTexture(ghost_texture_pink_down);
    SDL_DestroyTexture(ghost_texture_pink_up);
    SDL_DestroyTexture(ghost_texture_pink_left);
    SDL_DestroyTexture(ghost_texture_pink_right);

    SDL_DestroyTexture(ghost_texture_red_up);
    SDL_DestroyTexture(ghost_texture_red_down);
    SDL_DestroyTexture(ghost_texture_red_left);
    SDL_DestroyTexture(ghost_texture_red_right);

    SDL_DestroyTexture(map_texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(screen);
    

    // Quit the SDL program
    SDL_Quit();

    return EXIT_SUCCESS;
}
