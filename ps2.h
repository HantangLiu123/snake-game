#ifndef PS2_H
#define PS2_H

#include <stdint.h>

// Input command buffer size
#define INPUT_BUF_SIZE 16

// Buffered input command types
typedef enum
{
    CMD_NONE = 0,
    CMD_UP,
    CMD_LEFT,
    CMD_DOWN,
    CMD_RIGHT,
    CMD_SPACE
} InputCmd;

void ps2_init(void);
void ps2_apply_buffered_input(volatile int *game_running);

// pop an input out of the buffer, if the cmd popped out is CMD_NONE,
// the buffer is already empty
InputCmd pop_input(void);
void handler(void) __attribute__((interrupt("machine")));

#endif