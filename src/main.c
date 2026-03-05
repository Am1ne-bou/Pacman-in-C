#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include <time.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>

#include "map.h"
#include "ghost.h"
#include "utils.h"

// How many ms before vulnerable ends to start flickering
#define FLICKER_WARNING_MS 2000
// Flicker interval in ms (toggle every N ms)
#define FLICKER_INTERVAL_MS 200

int main(int argc, char* argv[])
{
    srand(time(NULL)); 
    enum tile_type_e** map = NULL;
    int map_width, map_height;
    if (argc > 1) {
        if (!get_map_size(argv[1], &map_height, &map_width) ||
            !allocate_map(&map, map_height, map_width) ||
            !load_map_from_file(argv[1], map, &map_height, &map_width))
        {
            fprintf(stderr, "Could not get map\n");
            map_height = N_Y_TILES;
            map_width = N_X_TILES;
            if (!allocate_map(&map, map_height, map_width))
                fprintf(stderr, "Could not allocate memory for map\n");
            init_map(map, &map_height, &map_width);
            fprintf(stderr, "Using default map\n");
        }
    } else {
        map_height = N_Y_TILES;
        map_width = N_X_TILES;
        if (!allocate_map(&map, map_height, map_width))
            fprintf(stderr, "Could not allocate memory for map\n");
        init_map(map, &map_height, &map_width);
    }

    int ret;
    int is_running = 1;
    const int tile_size = PACMAN_SIZE + 8;
    SDL_Rect window_rect = {
        .w = map_width * tile_size,
        .h = map_height * tile_size,
        .x = SDL_WINDOWPOS_UNDEFINED,
        .y = SDL_WINDOWPOS_UNDEFINED,
    };

    int move_direction = 0;
    int requested_direction = 0;
    bool pacman_open = true;
    const Uint32 toggle_delay = 150;
    int score = 0;
    int cycle_pacgomme = 0;
    Uint32 vulnerable_start_time = 0;
    float speed_pacman = 1.0f;
    float pacman_acc_x = 0.0f, pacman_acc_y = 0.0f;
#define SPEED_GHOST 1.0f
    int vulnerable = 0;
    // 1 = playing, 0 = game over (lost), 2 = win
    int game_on = 1;

    ret = SDL_Init(SDL_INIT_VIDEO);
    if (ret != 0) {
        fprintf(stderr, "Could not init SDL: %s\n", SDL_GetError());
        free_map(map, map_height);
        return EXIT_FAILURE;
    }

    if (TTF_Init() != 0) {
        fprintf(stderr, "Could not init TTF: %s\n", TTF_GetError());
        free_map(map, map_height);
        return EXIT_FAILURE;
    }

    TTF_Font* font = TTF_OpenFont("/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf", 24);
    if (!font) {
        fprintf(stderr, "Could not load font: %s\n", TTF_GetError());
        free_map(map, map_height);
        return EXIT_FAILURE;
    }

    Uint32 last_toggle_time = SDL_GetTicks();

    set_window_position_coords(&window_rect);
    SDL_Window *screen = SDL_CreateWindow(
        "Pacman",
        window_rect.x, window_rect.y,
        window_rect.w * RENDER_SCALE, window_rect.h * RENDER_SCALE,
        0
    );
    if (!screen) {
        fprintf(stderr, "Could not create SDL screen: %s\n", SDL_GetError());
        free_map(map, map_height);
        return EXIT_FAILURE;
    }

    SDL_Renderer *renderer = SDL_CreateRenderer(screen, -1, SDL_RENDERER_SOFTWARE);
    if (!renderer) {
        fprintf(stderr, "Could not create SDL renderer: %s\n", SDL_GetError());
        free_map(map, map_height);
        return EXIT_FAILURE;
    }

    ret = SDL_RenderSetScale(renderer, RENDER_SCALE, RENDER_SCALE);
    if (ret < 0) {
        fprintf(stderr, "Could not scale SDL renderer: %s\n", SDL_GetError());
        free_map(map, map_height);
        return EXIT_FAILURE;
    }

    // Load pacman textures
    SDL_Texture* pacman_texture_left  = IMG_LoadTexture(renderer, "images/pacman-left.png");
    SDL_Texture* pacman_texture_right = IMG_LoadTexture(renderer, "images/pacman-right.png");
    SDL_Texture* pacman_texture_up    = IMG_LoadTexture(renderer, "images/pacman-up.png");
    SDL_Texture* pacman_texture_down  = IMG_LoadTexture(renderer, "images/pacman-down.png");
    SDL_Texture* pacman_texture_full  = IMG_LoadTexture(renderer, "images/pacman-full.png");

    // Load ghost textures
    SDL_Texture* ghost_texture_pink_up    = IMG_LoadTexture(renderer, "images/ghost-pink-up.png");
    SDL_Texture* ghost_texture_pink_down  = IMG_LoadTexture(renderer, "images/ghost-pink-down.png");
    SDL_Texture* ghost_texture_pink_left  = IMG_LoadTexture(renderer, "images/ghost-pink-left.png");
    SDL_Texture* ghost_texture_pink_right = IMG_LoadTexture(renderer, "images/ghost-pink-right.png");

    SDL_Texture* ghost_texture_red_up    = IMG_LoadTexture(renderer, "images/ghost-red-up.png");
    SDL_Texture* ghost_texture_red_down  = IMG_LoadTexture(renderer, "images/ghost-red-down.png");
    SDL_Texture* ghost_texture_red_left  = IMG_LoadTexture(renderer, "images/ghost-red-left.png");
    SDL_Texture* ghost_texture_red_right = IMG_LoadTexture(renderer, "images/ghost-red-right.png");

    SDL_Texture* ghost_texture_blue_up    = IMG_LoadTexture(renderer, "images/ghost-blue-up.png");
    SDL_Texture* ghost_texture_blue_down  = IMG_LoadTexture(renderer, "images/ghost-blue-down.png");
    SDL_Texture* ghost_texture_blue_left  = IMG_LoadTexture(renderer, "images/ghost-blue-left.png");
    SDL_Texture* ghost_texture_blue_right = IMG_LoadTexture(renderer, "images/ghost-blue-right.png");

    SDL_Texture* ghost_texture_orange_up    = IMG_LoadTexture(renderer, "images/ghost-orange-up.png");
    SDL_Texture* ghost_texture_orange_down  = IMG_LoadTexture(renderer, "images/ghost-orange-down.png");
    SDL_Texture* ghost_texture_orange_left  = IMG_LoadTexture(renderer, "images/ghost-orange-left.png");
    SDL_Texture* ghost_texture_orange_right = IMG_LoadTexture(renderer, "images/ghost-orange-right.png");

    // ghost-blue-dizzy = vulnerable texture, ghost-white-dizzy = flicker texture
    SDL_Texture* ghost_texture_vulnerable = IMG_LoadTexture(renderer, "images/ghost-blue-dizzy.png");
    SDL_Texture* ghost_texture_flicker    = IMG_LoadTexture(renderer, "images/ghost-white-dizzy.png");
    SDL_Texture* ghost_texture_eyes       = IMG_LoadTexture(renderer, "images/ghost-eyes.png");

    if (!pacman_texture_left||!pacman_texture_right||!pacman_texture_up||!pacman_texture_down||!pacman_texture_full||
        !ghost_texture_pink_up||!ghost_texture_pink_down||!ghost_texture_pink_left||!ghost_texture_pink_right||
        !ghost_texture_red_up||!ghost_texture_red_down||!ghost_texture_red_left||!ghost_texture_red_right||
        !ghost_texture_orange_up||!ghost_texture_orange_down||!ghost_texture_orange_left||!ghost_texture_orange_right||
        !ghost_texture_blue_up||!ghost_texture_blue_down||!ghost_texture_blue_left||!ghost_texture_blue_right||
        !ghost_texture_vulnerable||!ghost_texture_flicker||!ghost_texture_eyes)
    {
        fprintf(stderr, "Could not load image: %s\n", SDL_GetError());
        free_map(map, map_height);
        return EXIT_FAILURE;
    }

    SDL_Texture* current_pacman_texture       = pacman_texture_right;
    SDL_Texture* current_pink_ghost_texture   = ghost_texture_pink_up;
    SDL_Texture* current_red_ghost_texture    = ghost_texture_red_up;
    SDL_Texture* current_blue_ghost_texture   = ghost_texture_blue_up;
    SDL_Texture* current_orange_ghost_texture = ghost_texture_orange_up;

    SDL_Rect pacman_position = { .w = PACMAN_SIZE, .h = PACMAN_SIZE };
    SDL_Rect map_position    = { .x = 0, .y = 0, .w = window_rect.w, .h = window_rect.h };

    int taillePacgomme = TILE_SIZE / 3;
    SDL_Rect posgomme = { .w = taillePacgomme, .h = taillePacgomme };

    // Count total gommes for win condition
    int tab_pos_gommes[map_height][map_width];
    int total_gommes = 0;
    for (int i = 0; i < map_height; i++) {
        for (int j = 0; j < map_width; j++) {
            if (map[i][j] == WALL) {
                tab_pos_gommes[i][j] = 0;
            } else {
                tab_pos_gommes[i][j] = 1;
                total_gommes++;
            }
        }
    }

    // Find ghost house center: average position of all ghost starts
    // Fallback: top-right corner if no ghost house found
    int ghost_house_x = (map_width - 2) * tile_size + (tile_size - PACMAN_SIZE) / 2;
    int ghost_house_y = (tile_size - PACMAN_SIZE) / 2;
    int ghost_tile_count = 0;
    int ghost_tile_sum_x = 0, ghost_tile_sum_y = 0;
    for (int y = 0; y < map_height; y++) {
        for (int x = 0; x < map_width; x++) {
            if (map[y][x] >= GHOST_pink_START && map[y][x] <= GHOST_orange_START) {
                ghost_tile_sum_x += x * tile_size + (tile_size - PACMAN_SIZE) / 2;
                ghost_tile_sum_y += y * tile_size + (tile_size - PACMAN_SIZE) / 2;
                ghost_tile_count++;
            }
        }
    }
    if (ghost_tile_count > 0) {
        ghost_house_x = ghost_tile_sum_x / ghost_tile_count;
        ghost_house_y = ghost_tile_sum_y / ghost_tile_count;
    }

    struct Ghost pink_ghost = {
        .position = {.w = PACMAN_SIZE, .h = PACMAN_SIZE},
        .direction = SDLK_LEFT,
        .tex_up = ghost_texture_pink_up, .tex_down = ghost_texture_pink_down,
        .tex_left = ghost_texture_pink_left, .tex_right = ghost_texture_pink_right,
        .current_tex = &current_pink_ghost_texture,
        .speed = SPEED_GHOST, .acc_x = 0.0f, .acc_y = 0.0f, .exists = 1, .chase = 2,
        .returning = 0, .spawn_x = ghost_house_x, .spawn_y = ghost_house_y,
        .tex_eyes = ghost_texture_eyes,
    };
    struct Ghost red_ghost = {
        .position = {.w = PACMAN_SIZE, .h = PACMAN_SIZE},
        .direction = SDLK_LEFT,
        .tex_up = ghost_texture_red_up, .tex_down = ghost_texture_red_down,
        .tex_left = ghost_texture_red_left, .tex_right = ghost_texture_red_right,
        .current_tex = &current_red_ghost_texture,
        .speed = SPEED_GHOST, .acc_x = 0.0f, .acc_y = 0.0f, .exists = 1, .chase = 1,
        .returning = 0, .spawn_x = ghost_house_x, .spawn_y = ghost_house_y,
        .tex_eyes = ghost_texture_eyes,
    };
    struct Ghost blue_ghost = {
        .position = {.w = PACMAN_SIZE, .h = PACMAN_SIZE},
        .direction = SDLK_LEFT,
        .tex_up = ghost_texture_blue_up, .tex_down = ghost_texture_blue_down,
        .tex_left = ghost_texture_blue_left, .tex_right = ghost_texture_blue_right,
        .current_tex = &current_blue_ghost_texture,
        .speed = SPEED_GHOST, .acc_x = 0.0f, .acc_y = 0.0f, .exists = 1, .chase = 0,
        .returning = 0, .spawn_x = ghost_house_x, .spawn_y = ghost_house_y,
        .tex_eyes = ghost_texture_eyes,
    };
    struct Ghost orange_ghost = {
        .position = {.w = PACMAN_SIZE, .h = PACMAN_SIZE},
        .direction = SDLK_LEFT,
        .tex_up = ghost_texture_orange_up, .tex_down = ghost_texture_orange_down,
        .tex_left = ghost_texture_orange_left, .tex_right = ghost_texture_orange_right,
        .current_tex = &current_orange_ghost_texture,
        .speed = SPEED_GHOST, .acc_x = 0.0f, .acc_y = 0.0f, .exists = 1, .chase = 0,
        .returning = 0, .spawn_x = ghost_house_x, .spawn_y = ghost_house_y,
        .tex_eyes = ghost_texture_eyes,
    };

    struct Ghost ghosts[4] = { pink_ghost, red_ghost, blue_ghost, orange_ghost };

    SDL_Texture* map_texture = SDL_CreateTexture(
        renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET,
        window_rect.w, window_rect.h
    );

    SDL_SetRenderTarget(renderer, map_texture);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    SDL_SetRenderDrawColor(renderer, 192, 188, 185, 255);
    for (int y = 0; y < map_height; y++)
    {
        for (int x = 0; x < map_width; x++)
        {
            if (map[y][x] == WALL)
            {
                SDL_Rect rectangle = {
                    .x = x*tile_size, .y = y*tile_size,
                    .w = tile_size,   .h = tile_size,
                };
                SDL_RenderFillRect(renderer, &rectangle);
            }
            else if (map[y][x] == PACMAN_START)
            {
                pacman_position.x = x*tile_size + (tile_size-PACMAN_SIZE)/2;
                pacman_position.y = y*tile_size + (tile_size-PACMAN_SIZE)/2;
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

    SDL_SetRenderTarget(renderer, NULL);

    // Cached end-screen textures (rebuilt once when game ends)
    SDL_Texture* cached_title_tex   = NULL;
    SDL_Rect     cached_title_rect  = {0};
    SDL_Texture* cached_fscore_tex  = NULL;
    SDL_Rect     cached_fscore_rect = {0};
    int          cached_game_on_val = 0;
    int          cached_score_val   = -1;

    // Cached score HUD texture (rebuilt only when score changes)
    SDL_Texture* cached_score_hud_tex  = NULL;
    SDL_Rect     cached_score_hud_rect = {0};
    int          cached_hud_score_val  = -1;

    while (is_running)
    {
        SDL_Event event;
        SDL_PollEvent(&event);

        switch (event.type) {
            case SDL_QUIT: is_running = 0; break;
        }

        if (game_on == 1)
        {
            SDL_RenderClear(renderer);
            SDL_RenderCopy(renderer, map_texture, NULL, &map_position);

            switch (event.type)
            {
                case SDL_QUIT: is_running = 0; break;
                case SDL_KEYDOWN:
                    switch (event.key.keysym.sym)
                    {
                        case SDLK_q:     is_running = 0; break;
                        case SDLK_UP:    case SDLK_w: requested_direction = SDLK_UP;    break;
                        case SDLK_DOWN:  case SDLK_s: requested_direction = SDLK_DOWN;  break;
                        case SDLK_LEFT:  case SDLK_a: requested_direction = SDLK_LEFT;  break;
                        case SDLK_RIGHT: case SDLK_d: requested_direction = SDLK_RIGHT; break;
                    }
                    break;
            }

            move_entity(&pacman_position, &move_direction, &requested_direction, &speed_pacman,
                &pacman_acc_x, &pacman_acc_y,
                map, map_height, map_width, &current_pacman_texture,
                pacman_texture_up, pacman_texture_down,
                pacman_texture_left, pacman_texture_right);

            Uint32 current_time = SDL_GetTicks();
            if (current_time - last_toggle_time >= toggle_delay) {
                pacman_open = !pacman_open;
                last_toggle_time = current_time;
            }
            SDL_Texture* texture_to_render = pacman_open ? current_pacman_texture : pacman_texture_full;
            SDL_RenderCopy(renderer, texture_to_render, NULL, &pacman_position);

            // Compute flicker: true when close to end of vulnerable time
            int flicker = 0;
            if (vulnerable == 1 && vulnerable_start_time > 0) {
                Uint32 elapsed = SDL_GetTicks() - vulnerable_start_time;
                Uint32 remaining = (elapsed < VULNERABLE_TIME) ? (VULNERABLE_TIME - elapsed) : 0;
                if (remaining < FLICKER_WARNING_MS) {
                    flicker = (SDL_GetTicks() / FLICKER_INTERVAL_MS) % 2;
                }
            }

            // Dynamic ghost speed based on % of gommes eaten (float, smooth steps):
            // Vulnerable -> 0.6 (slow)
            float ghost_normal_speed;
            float eaten_ratio = (total_gommes > 0) ? ((float)score / total_gommes) : 0.0f;
            if      (eaten_ratio < 0.50f) ghost_normal_speed = 1.0f;
            else if (eaten_ratio < 0.65f) ghost_normal_speed = 1.1f;
            else if (eaten_ratio < 0.80f) ghost_normal_speed = 1.2f;
            else                          ghost_normal_speed = 1.4f;

            for (int i = 0; i < 4; i++) {
                if (!ghosts[i].returning)
                    ghosts[i].speed = (vulnerable == 1) ? 0.6f : ghost_normal_speed;
            }

            // Update ghosts
            for (int i = 0; i < 4; i++)
                updating_ghost(&ghosts[i], ghost_texture_vulnerable, ghost_texture_flicker,
                    renderer, &pacman_position, &move_direction,
                    map, map_height, map_width, &vulnerable, &game_on, flicker);

            // EAT
            int pos_tile_pacman_x, pos_tile_pacman_y;
            pixelToTile(pacman_position.x, pacman_position.y, &pos_tile_pacman_x, &pos_tile_pacman_y);
            if (tab_pos_gommes[pos_tile_pacman_y][pos_tile_pacman_x] == 1) {
                tab_pos_gommes[pos_tile_pacman_y][pos_tile_pacman_x] = 0;
                score++;
                cycle_pacgomme++;
            }

            // Win condition
            if (score >= total_gommes)
                game_on = 2;

            if (cycle_pacgomme > PACGOMME_CYCLE && vulnerable == 0) {
                vulnerable_start_time = SDL_GetTicks();
                vulnerable = 1;
            }
            if (vulnerable == 1 && (SDL_GetTicks() - vulnerable_start_time >= VULNERABLE_TIME)) {
                vulnerable = 0;
                vulnerable_start_time = 0;
                cycle_pacgomme = 0;
            }

            // Draw gommes
            for (int i = 0; i < map_height; i++) {
                for (int j = 0; j < map_width; j++) {
                    if (tab_pos_gommes[i][j] == 1) {
                        posgomme.x = j * tile_size + (tile_size - posgomme.w) / 2;
                        posgomme.y = i * tile_size + (tile_size - posgomme.h) / 2;
                        SDL_RenderCopy(renderer, pacman_texture_full, NULL, &posgomme);
                    }
                }
            }

            // Draw score HUD (cached, rebuilt only when score changes)
            int remaining = total_gommes - score;
            if (score != cached_hud_score_val) {
                if (cached_score_hud_tex) SDL_DestroyTexture(cached_score_hud_tex);
                char score_text[64];
                snprintf(score_text, sizeof(score_text), "SCORE: %d  |  BALLS: %d", score, remaining);
                SDL_Color white = {255, 255, 255, 255};
                SDL_Surface* score_surface = TTF_RenderText_Solid(font, score_text, white);
                cached_score_hud_tex = SDL_CreateTextureFromSurface(renderer, score_surface);
                cached_score_hud_rect = (SDL_Rect){10, 5, score_surface->w, score_surface->h};
                SDL_FreeSurface(score_surface);
                cached_hud_score_val = score;
            }
            SDL_RenderCopy(renderer, cached_score_hud_tex, NULL, &cached_score_hud_rect);

            SDL_RenderPresent(renderer);
            SDL_Delay(2);
        }
        else
        {
            // Game over / win screen
            SDL_RenderClear(renderer);
            SDL_RenderCopy(renderer, map_texture, NULL, &map_position);
            SDL_RenderCopy(renderer, current_pacman_texture, NULL, &pacman_position);
            SDL_RenderCopy(renderer, *ghosts[0].current_tex, NULL, &ghosts[0].position);
            SDL_RenderCopy(renderer, *ghosts[1].current_tex, NULL, &ghosts[1].position);
            SDL_RenderCopy(renderer, *ghosts[2].current_tex, NULL, &ghosts[2].position);
            SDL_RenderCopy(renderer, *ghosts[3].current_tex, NULL, &ghosts[3].position);

            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 160);
            SDL_Rect overlay = {0, 0, window_rect.w, window_rect.h};
            SDL_RenderFillRect(renderer, &overlay);

            // Build end-screen textures once (cached)
            if (cached_game_on_val != game_on || cached_score_val != score) {
                if (cached_title_tex)  SDL_DestroyTexture(cached_title_tex);
                if (cached_fscore_tex) SDL_DestroyTexture(cached_fscore_tex);

                SDL_Color title_color = (game_on == 2)
                    ? (SDL_Color){50, 220, 50, 255}
                    : (SDL_Color){220, 50, 50, 255};
                const char* title_text = (game_on == 2) ? "YOU WIN!" : "GAME OVER";
                SDL_Surface* title_surface = TTF_RenderText_Solid(font, title_text, title_color);
                cached_title_tex  = SDL_CreateTextureFromSurface(renderer, title_surface);
                cached_title_rect = (SDL_Rect){
                    (window_rect.w - title_surface->w) / 2,
                    window_rect.h / 2 - title_surface->h - 10,
                    title_surface->w, title_surface->h
                };
                SDL_FreeSurface(title_surface);

                char final_score[32];
                snprintf(final_score, sizeof(final_score), "SCORE: %d", score);
                SDL_Color white = {255, 255, 255, 255};
                SDL_Surface* fscore_surface = TTF_RenderText_Solid(font, final_score, white);
                cached_fscore_tex  = SDL_CreateTextureFromSurface(renderer, fscore_surface);
                cached_fscore_rect = (SDL_Rect){
                    (window_rect.w - fscore_surface->w) / 2,
                    window_rect.h / 2 + 10,
                    fscore_surface->w, fscore_surface->h
                };
                SDL_FreeSurface(fscore_surface);

                cached_game_on_val = game_on;
                cached_score_val   = score;
            }
            SDL_RenderCopy(renderer, cached_title_tex,  NULL, &cached_title_rect);
            SDL_RenderCopy(renderer, cached_fscore_tex, NULL, &cached_fscore_rect);

            SDL_RenderPresent(renderer);
            SDL_Delay(2);
        }
    }

    free_map(map, map_height);

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
    SDL_DestroyTexture(ghost_texture_vulnerable);
    SDL_DestroyTexture(ghost_texture_flicker);
    SDL_DestroyTexture(ghost_texture_eyes);
    SDL_DestroyTexture(map_texture);
    if (cached_title_tex)      SDL_DestroyTexture(cached_title_tex);
    if (cached_fscore_tex)     SDL_DestroyTexture(cached_fscore_tex);
    if (cached_score_hud_tex)  SDL_DestroyTexture(cached_score_hud_tex);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(screen);

    TTF_CloseFont(font);
    TTF_Quit();
    IMG_Quit();
    SDL_Quit();

    return EXIT_SUCCESS;
}