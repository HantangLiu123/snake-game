#ifndef PS2_H
#define PS2_H

#include <stdint.h>

#define DIR_RIGHT 0
#define DIR_DOWN  1
#define DIR_LEFT  2
#define DIR_UP    3

extern volatile int snake_dir;
extern volatile int pending_turn;

void ps2_init(void);
void ps2_apply_turn(void);
void handler(void) __attribute__((interrupt("machine")));

#endif