#ifndef GHOST_H
#define GHOST_H

#include <SDL2/SDL.h>
#include "map.h"

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
    int exists;      // 1 = alive, 0 = eaten (returning as eyes)
    int chase;
    int returning;   // 1 = ghost was eaten, travelling as eyes back to spawn
    int spawn_x;     // pixel x of spawn position
    int spawn_y;     // pixel y of spawn position
    SDL_Texture* tex_eyes;
};

int get_ghost_direction_randomly(SDL_Rect* ghost_pos, int current_dir, enum tile_type_e** map, int height, int width);
int get_ghost_direction_chase(SDL_Rect* ghost_pos, SDL_Rect* pacman_pos, int pacman_current_dir, int current_dir, enum tile_type_e** map, int height, int width, int* mode_chase);
int get_ghost_direction_escape(SDL_Rect* ghost_pos, SDL_Rect* pacman_pos, enum tile_type_e** map, int height, int width, int current_dir);
void updating_ghost(struct Ghost* ghost, SDL_Texture* vulnerable_tex, SDL_Texture* flicker_tex,
    SDL_Renderer* renderer, SDL_Rect* pacman_pos, int* pacman_current_dir,
    enum tile_type_e** map, int height, int width,
    int* vulnerable, int* game_on, int flicker);

#endif