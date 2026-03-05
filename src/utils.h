#ifndef UTILS_H
#define UTILS_H

#include <stdbool.h>
#include <SDL2/SDL.h>
#include "map.h"

#define RENDER_SCALE 0.7

#define PACMAN_SIZE 32
#define TILE_SIZE (PACMAN_SIZE+8)

void pixelToTile(int pixelX, int pixelY, int *tileX, int *tileY);
void TileTopixel(int tileX, int tileY, int *pixelX, int *pixelY);
bool can_move(int tileX, int tileY, int direction, enum tile_type_e** map, int height, int width);
void set_entity_texture(SDL_Texture** current, int direction,
                        SDL_Texture* up, SDL_Texture* down,
                        SDL_Texture* left, SDL_Texture* right);
void move_entity(SDL_Rect* entity, int* move_dir, int* requested_dir, float* speed,
                float* acc_x, float* acc_y,
                enum tile_type_e** map, int height, int width,
                SDL_Texture** current_texture,
                SDL_Texture* tex_up, SDL_Texture* tex_down,
                SDL_Texture* tex_left, SDL_Texture* tex_right);
int distance_between_two_entities(SDL_Rect* entity1, SDL_Rect* entity2);
void set_window_position_coords(SDL_Rect* window_rect);

#endif