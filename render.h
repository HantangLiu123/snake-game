#ifndef RENDER_H
#define RENDER_H

#include "game.h"
#include <stdbool.h>

void init_vga();

// draw the whole grid, but need to be called twice due to double buffer
void draw_whole_grid();

// update the snake from its last position to its current position.
// creates the animation of the snake going from its last position
// to the current position.
void update_snake(const Coordinate *snake_body);

// need to call all the time before rendering a snake, but won't change
// anything on the vga
void init_snake();

// draw an apple at the game coordinate on both buffer
void draw_apple(int game_x, int game_y);

// a snake death animation updating the snake from last position to death position
// the hit_on_wall indicates that whether the snake dies due to hitting on wall
// or hit by itself
void update_snake_death(const Coordinate *snake_body, bool hit_on_wall);

// interface for drawing / clearing the apple on the side bar
// a figure indicating the digit beside it is the apple consumed
void draw_apple_side_bar();
void clear_apple_side_bar();

// interface for updating / clearing the digit of apple consumed
void update_digit(int num);
void clear_digit();

// interface for updating the game status showing on the side bar
void update_status(GameStatus status);

void draw_gameover_img();
void draw_win_img();

#endif
