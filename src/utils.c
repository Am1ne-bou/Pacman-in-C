#include <SDL2/SDL.h>
#include "utils.h"
#include "map.h"

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
