#ifndef PS2_H
#define PS2_H

#include <stdint.h>

// Snake direction constants
#define DIR_RIGHT 0
#define DIR_DOWN  1
#define DIR_LEFT  2
#define DIR_UP    3

// Input command buffer size
#define INPUT_BUF_SIZE 16

// Buffered input command types
typedef enum {
    CMD_NONE = 0,
    CMD_UP,
    CMD_LEFT,
    CMD_DOWN,
    CMD_RIGHT,
    CMD_SPACE
} InputCmd;

extern volatile int snake_dir;

void ps2_init(void);
void ps2_apply_buffered_input(volatile int *game_running);
void handler(void) __attribute__((interrupt("machine")));

#endif