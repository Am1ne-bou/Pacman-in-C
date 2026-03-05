#include <stdlib.h>
#include <SDL2/SDL.h>
#include "ghost.h"
#include "utils.h"
#include "map.h"

// Function to get the ghost direction randomly
int get_ghost_direction_randomly(SDL_Rect* ghost_pos, int current_dir, enum tile_type_e** map, int height, int width)     
{       
        int centerX = ghost_pos->x + PACMAN_SIZE / 2;
        int centerY = ghost_pos->y + PACMAN_SIZE / 2;
        int tileX, tileY;
        pixelToTile(centerX, centerY, &tileX, &tileY);
        
        int tileCenterX = tileX * TILE_SIZE + TILE_SIZE / 2;
        int tileCenterY = tileY * TILE_SIZE + TILE_SIZE / 2;
        
        if (centerX != tileCenterX || centerY != tileCenterY)
            return current_dir; 

        int directions[] = {SDLK_UP, SDLK_DOWN, SDLK_LEFT, SDLK_RIGHT};
        int opposite_dir;

        switch(current_dir) {
            case SDLK_UP:    opposite_dir = SDLK_DOWN;  break;
            case SDLK_DOWN:  opposite_dir = SDLK_UP;    break;
            case SDLK_LEFT:  opposite_dir = SDLK_RIGHT; break;
            case SDLK_RIGHT: opposite_dir = SDLK_LEFT;  break;
            default:         opposite_dir = -1;          break; 
        }

        // Try directions in random order (note: may test same dir twice)
        for (int i = 0; i < 4; i++) 
        {
            int test_dir = directions[rand() % 4];
            if (test_dir == opposite_dir) continue;

            int tx, ty;
            pixelToTile(ghost_pos->x + PACMAN_SIZE/2, ghost_pos->y + PACMAN_SIZE/2, &tx, &ty);
            if (can_move(tx, ty, test_dir, map, height, width))
                return test_dir;
        }

        return current_dir;
}

// Function to get the ghost direction when chasing a target position
int get_ghost_direction_chase(SDL_Rect* ghost_pos, SDL_Rect* pacman_pos, int pacman_current_dir, int current_dir, enum tile_type_e** map, int height, int width, int* mode_chase)     
{       
        int centerX = ghost_pos->x + PACMAN_SIZE / 2;
        int centerY = ghost_pos->y + PACMAN_SIZE / 2;
        int tileX, tileY;
        pixelToTile(centerX, centerY, &tileX, &tileY);
        
        int tileCenterX = tileX * TILE_SIZE + TILE_SIZE / 2;
        int tileCenterY = tileY * TILE_SIZE + TILE_SIZE / 2;
        
        if (centerX != tileCenterX || centerY != tileCenterY)
            return current_dir; 

        int directions[] = {SDLK_UP, SDLK_DOWN, SDLK_LEFT, SDLK_RIGHT};
        int opposite_dir;
        int dx = 0, dy = 0;

        switch(current_dir) {
            case SDLK_UP:    opposite_dir = SDLK_DOWN;  break;
            case SDLK_DOWN:  opposite_dir = SDLK_UP;    break;
            case SDLK_LEFT:  opposite_dir = SDLK_RIGHT; break;
            case SDLK_RIGHT: opposite_dir = SDLK_LEFT;  break;
            default:         opposite_dir = -1;          break; 
        }

        switch(pacman_current_dir) {
            case SDLK_UP:    dy = -4*TILE_SIZE; break;
            case SDLK_DOWN:  dy =  4*TILE_SIZE; break;
            case SDLK_LEFT:  dx = -4*TILE_SIZE; break;
            case SDLK_RIGHT: dx =  4*TILE_SIZE; break;
            default: dx = 0; dy = 0; break;
        }

        SDL_Rect pacman_pos_copy = *pacman_pos;
        pacman_pos_copy.x = pacman_pos->x + dx;
        pacman_pos_copy.y = pacman_pos->y + dy;

        int best_dir = current_dir;
        int min_dist = 10000000;
        
        for (int i = 0; i < 4; i++) 
        {
            int test_dir = directions[i];
            if (test_dir == opposite_dir) continue;

            int tx, ty;
            pixelToTile(ghost_pos->x + PACMAN_SIZE/2, ghost_pos->y + PACMAN_SIZE/2, &tx, &ty);

            if (can_move(tx, ty, test_dir, map, height, width)) {
                SDL_Rect next_ghost_pos = *ghost_pos;
                switch (test_dir) {
                    case SDLK_UP:    next_ghost_pos.y -= TILE_SIZE; break;
                    case SDLK_DOWN:  next_ghost_pos.y += TILE_SIZE; break;
                    case SDLK_LEFT:  next_ghost_pos.x -= TILE_SIZE; break;
                    case SDLK_RIGHT: next_ghost_pos.x += TILE_SIZE; break;
                }

                int distance = 0;
                if (*mode_chase == 1)
                    distance = distance_between_two_entities(&next_ghost_pos, pacman_pos);
                if (*mode_chase == 2)
                    distance = distance_between_two_entities(&next_ghost_pos, &pacman_pos_copy);
                                                
                if (distance < min_dist) {
                    min_dist = distance;
                    best_dir = test_dir;
                }
            }
        }

        return best_dir;
}

// Function to get the ghost direction when vulnerable (flee from pacman)
int get_ghost_direction_escape(SDL_Rect* ghost_pos, SDL_Rect* pacman_pos, enum tile_type_e** map, int height, int width, int current_dir)
{
        int centerX = ghost_pos->x + PACMAN_SIZE / 2;
        int centerY = ghost_pos->y + PACMAN_SIZE / 2;
        int tileX, tileY;
        pixelToTile(centerX, centerY, &tileX, &tileY);
        
        int tileCenterX = tileX * TILE_SIZE + TILE_SIZE / 2;
        int tileCenterY = tileY * TILE_SIZE + TILE_SIZE / 2;
        
        if (centerX != tileCenterX || centerY != tileCenterY)
            return current_dir; 

        int directions[] = {SDLK_UP, SDLK_DOWN, SDLK_LEFT, SDLK_RIGHT};
        int opposite_dir;

        switch(current_dir) {
            case SDLK_UP:    opposite_dir = SDLK_DOWN;  break;
            case SDLK_DOWN:  opposite_dir = SDLK_UP;    break;
            case SDLK_LEFT:  opposite_dir = SDLK_RIGHT; break;
            case SDLK_RIGHT: opposite_dir = SDLK_LEFT;  break;
            default:         opposite_dir = -1;          break; 
        }

        int best_dir = current_dir;
        int max_dist = -10;
        
        for (int i = 0; i < 4; i++) 
        {
            int test_dir = directions[i];
            if (test_dir == opposite_dir) continue;

            int tx, ty;
            pixelToTile(ghost_pos->x + PACMAN_SIZE/2, ghost_pos->y + PACMAN_SIZE/2, &tx, &ty);

            if (can_move(tx, ty, test_dir, map, height, width)) {
                SDL_Rect next_ghost_pos = *ghost_pos;
                switch (test_dir) {
                    case SDLK_UP:    next_ghost_pos.y -= TILE_SIZE; break;
                    case SDLK_DOWN:  next_ghost_pos.y += TILE_SIZE; break;
                    case SDLK_LEFT:  next_ghost_pos.x -= TILE_SIZE; break;
                    case SDLK_RIGHT: next_ghost_pos.x += TILE_SIZE; break;
                }

                int distance = distance_between_two_entities(&next_ghost_pos, pacman_pos);
                if (distance > max_dist) {
                    max_dist = distance;
                    best_dir = test_dir;
                }
            }
        }

        return best_dir;
}

// Get direction toward a target pixel position (used for eyes returning to spawn)
static int get_direction_toward(SDL_Rect* ghost_pos, int target_x, int target_y,
    int current_dir, enum tile_type_e** map, int height, int width)
{
        int centerX = ghost_pos->x + PACMAN_SIZE / 2;
        int centerY = ghost_pos->y + PACMAN_SIZE / 2;
        int tileX, tileY;
        pixelToTile(centerX, centerY, &tileX, &tileY);

        int tileCenterX = tileX * TILE_SIZE + TILE_SIZE / 2;
        int tileCenterY = tileY * TILE_SIZE + TILE_SIZE / 2;

        if (centerX != tileCenterX || centerY != tileCenterY)
            return current_dir;

        int directions[] = {SDLK_UP, SDLK_DOWN, SDLK_LEFT, SDLK_RIGHT};
        int opposite_dir;
        switch(current_dir) {
            case SDLK_UP:    opposite_dir = SDLK_DOWN;  break;
            case SDLK_DOWN:  opposite_dir = SDLK_UP;    break;
            case SDLK_LEFT:  opposite_dir = SDLK_RIGHT; break;
            case SDLK_RIGHT: opposite_dir = SDLK_LEFT;  break;
            default:         opposite_dir = -1;          break;
        }

        // Build a fake target rect centered on spawn
        SDL_Rect target = { .x = target_x, .y = target_y, .w = PACMAN_SIZE, .h = PACMAN_SIZE };

        int best_dir = current_dir;
        int min_dist = 10000000;

        for (int i = 0; i < 4; i++) {
            int test_dir = directions[i];
            if (test_dir == opposite_dir) continue;

            int tx, ty;
            pixelToTile(ghost_pos->x + PACMAN_SIZE/2, ghost_pos->y + PACMAN_SIZE/2, &tx, &ty);

            if (can_move(tx, ty, test_dir, map, height, width)) {
                SDL_Rect next = *ghost_pos;
                switch (test_dir) {
                    case SDLK_UP:    next.y -= TILE_SIZE; break;
                    case SDLK_DOWN:  next.y += TILE_SIZE; break;
                    case SDLK_LEFT:  next.x -= TILE_SIZE; break;
                    case SDLK_RIGHT: next.x += TILE_SIZE; break;
                }
                int dist = distance_between_two_entities(&next, &target);
                if (dist < min_dist) {
                    min_dist = dist;
                    best_dir = test_dir;
                }
            }
        }

        return best_dir;
}

// Function to update the ghost position and check for collisions
void updating_ghost(struct Ghost* ghost, SDL_Texture* vulnerable_tex, SDL_Texture* flicker_tex,
    SDL_Renderer* renderer, SDL_Rect* pacman_pos, int* pacman_current_dir,
    enum tile_type_e** map, int height, int width,
    int* vulnerable, int* game_on, int flicker)
{
    // Ghost was eaten: travel as eyes back to spawn
    if (ghost->returning)
    {
        int dx = ghost->spawn_x - ghost->position.x;
        int dy = ghost->spawn_y - ghost->position.y;

        // Close enough to spawn: respawn as normal ghost
        if (dx * dx + dy * dy < TILE_SIZE * TILE_SIZE)
        {
            ghost->position.x = ghost->spawn_x;
            ghost->position.y = ghost->spawn_y;
            ghost->returning = 0;
            ghost->exists = 1;
            return;
        }

        // Move eyes toward spawn using chase logic
        ghost->direction = get_direction_toward(&ghost->position, ghost->spawn_x, ghost->spawn_y,
            ghost->direction, map, height, width);

        float eyes_speed = ghost->speed * 2.0f; // eyes move faster
        move_entity(&ghost->position, &ghost->direction, &ghost->direction, &eyes_speed,
            &ghost->acc_x, &ghost->acc_y,
            map, height, width, ghost->current_tex,
            ghost->tex_eyes, ghost->tex_eyes, ghost->tex_eyes, ghost->tex_eyes);

        SDL_RenderCopy(renderer, ghost->tex_eyes, NULL, &ghost->position);
        return;
    }

    // Ghost is dead and not returning: hide it
    if (!ghost->exists)
    {
        ghost->position.x = -100;
        ghost->position.y = -100;
        return;
    }

    // Normal ghost logic: get direction
    if (*vulnerable == 1)
    {
        ghost->direction = get_ghost_direction_escape(&ghost->position, pacman_pos, map, height, width, ghost->direction);
    }
    else
    {
        if (ghost->chase == 0)
            ghost->direction = get_ghost_direction_randomly(&ghost->position, ghost->direction, map, height, width);
        else
            ghost->direction = get_ghost_direction_chase(&ghost->position, pacman_pos, *pacman_current_dir,
                ghost->direction, map, height, width, &ghost->chase);
    }

    // Pick texture: normal, vulnerable (blue), or flickering (white)
    SDL_Texture* display_tex_up    = ghost->tex_up;
    SDL_Texture* display_tex_down  = ghost->tex_down;
    SDL_Texture* display_tex_left  = ghost->tex_left;
    SDL_Texture* display_tex_right = ghost->tex_right;

    if (*vulnerable == 1) {
        SDL_Texture* vtex = (flicker) ? flicker_tex : vulnerable_tex;
        display_tex_up = display_tex_down = display_tex_left = display_tex_right = vtex;
    }

    move_entity(&ghost->position, &ghost->direction, &ghost->direction, &ghost->speed,
        &ghost->acc_x, &ghost->acc_y,
        map, height, width, ghost->current_tex,
        display_tex_up, display_tex_down, display_tex_left, display_tex_right);

    SDL_RenderCopy(renderer, *ghost->current_tex, NULL, &ghost->position);

    // Collision with pacman
    if (SDL_HasIntersection(&ghost->position, pacman_pos)) {
        if (*vulnerable == 1) {
            // Ghost eaten: become eyes and return to spawn
            ghost->exists = 0;
            ghost->returning = 1;
        } else {
            *game_on = 0;
        }
    }
}