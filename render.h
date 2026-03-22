#ifndef RENDER_H
#define RENDER_H

#include "game.h"
#include <stdbool.h>

void init_vga();
void draw_whole_grid();
void update_snake(const Coordinate *snake_body);
void init_snake();
void draw_apple(int game_x, int game_y);
void update_snake_death(const Coordinate *snake_body, bool hit_on_wall);

void draw_apple_side_bar();
void clear_apple_side_bar();
void update_digit(int num);
void clear_digit();

typedef enum
{
    STATUS_END = 0,
    STATUS_RUN,
    STATUS_PAUSE
} GameStatus;

void update_status(GameStatus status);

#endif
