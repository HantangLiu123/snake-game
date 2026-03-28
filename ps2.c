#include "ps2.h"
#include "address_map.h"

volatile int snake_dir = DIR_RIGHT;   // Current actual snake direction

static volatile int ps2_break = 0;    // Set to 1 after receiving F0
static volatile InputCmd input_buf[INPUT_BUF_SIZE];   // Ring buffer for validated input commands
static volatile int buf_head = 0;     // Write index
static volatile int buf_tail = 0;     // Read index

// Push one validated command into the ring buffer
static void push_input(InputCmd cmd) {
    int next_head = (buf_head + 1) % INPUT_BUF_SIZE;

    // If buffer is full, drop the new command
    if (next_head == buf_tail)
        return;

    input_buf[buf_head] = cmd;
    buf_head = next_head;
}

// Pop one command from the ring buffer
static InputCmd pop_input(void) {
    InputCmd cmd;

    if (buf_head == buf_tail)
        return CMD_NONE;   // Buffer empty

    cmd = input_buf[buf_tail];
    buf_tail = (buf_tail + 1) % INPUT_BUF_SIZE;
    return cmd;
}

// PS/2 device ISR: only validate and buffer keystrokes
static void ps2_isr(void) {
    volatile int *PS2_ptr = (int *)PS2_BASE;
    int data;
    unsigned char byte;

    while (1) {
        data = *PS2_ptr;

        // bit15 = RVALID
        if ((data & 0x8000) == 0)
            break;

        byte = (unsigned char)(data & 0xFF);

        // F0 means the next byte is a released-key code
        if (byte == 0xF0) {
            ps2_break = 1;
            continue;
        }

        // Ignore released-key code
        if (ps2_break) {
            ps2_break = 0;
            continue;
        }

        // Buffer only make-codes that matter to the game
        if (byte == 0x1D) {          // W
            push_input(CMD_UP);
        } else if (byte == 0x1C) {   // A
            push_input(CMD_LEFT);
        } else if (byte == 0x1B) {   // S
            push_input(CMD_DOWN);
        } else if (byte == 0x23) {   // D
            push_input(CMD_RIGHT);
        } else if (byte == 0x29) {   // Space
            push_input(CMD_SPACE);
        }
    }
}

// Global trap handler
void handler(void) {
    int mcause_value;
    __asm__ volatile ("csrr %0, mcause" : "=r"(mcause_value));

    // PS/2 IRQ 22 -> mcause 0x80000016
    if (mcause_value == 0x80000016) {
        ps2_isr();
    }
}

// Initialize PS/2 device and enable its interrupt
void ps2_init(void) {
    volatile int *PS2_ptr = (int *)PS2_BASE;
    int data;
    int mstatus_value = 0b1000;
    int mtvec_value = (int)&handler;
    int mie_value;

    // Clear old bytes in PS/2 FIFO
    while (1) {
        data = *PS2_ptr;
        if ((data & 0x8000) == 0)
            break;
    }

    // Enable PS/2 receive interrupt at device level
    *(PS2_ptr + 1) = 0x1;

    // Disable global interrupts first
    __asm__ volatile ("csrc mstatus, %0" :: "r"(mstatus_value));

    // Set trap handler address
    __asm__ volatile ("csrw mtvec, %0" :: "r"(mtvec_value));

    // Clear currently enabled interrupt bits
    __asm__ volatile ("csrr %0, mie" : "=r"(mie_value));
    __asm__ volatile ("csrc mie, %0" :: "r"(mie_value));

    // Enable only IRQ 22 (PS/2)
    mie_value = (1 << 22);
    __asm__ volatile ("csrs mie, %0" :: "r"(mie_value));

    // Re-enable global interrupts
    __asm__ volatile ("csrs mstatus, %0" :: "r"(mstatus_value));
}

// Consume at most one buffered command each game tick
void ps2_apply_buffered_input(volatile int *game_running) {
    InputCmd cmd = pop_input();

    if (cmd == CMD_NONE)
        return;

    // Space toggles start/pause
    if (cmd == CMD_SPACE) {
        *game_running = !(*game_running);
        return;
    }

    // If moving horizontally, only W/S are valid
    if (snake_dir == DIR_LEFT || snake_dir == DIR_RIGHT) {
        if (cmd == CMD_UP)
            snake_dir = DIR_UP;
        else if (cmd == CMD_DOWN)
            snake_dir = DIR_DOWN;
    }
    // If moving vertically, only A/D are valid
    else if (snake_dir == DIR_UP || snake_dir == DIR_DOWN) {
        if (cmd == CMD_LEFT)
            snake_dir = DIR_LEFT;
        else if (cmd == CMD_RIGHT)
            snake_dir = DIR_RIGHT;
    }
}