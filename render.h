#ifndef RENDER_H
#define RENDER_H

#include "game.h"

void init_vga();
void draw_whole_grid();
void update_snake(const Coordinate *snake_body);
void init_snake();

#endif
