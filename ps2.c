#include "ps2.h"
#include "address_map.h"

volatile int snake_dir = DIR_RIGHT;
volatile int pending_turn = 0;

static volatile int ps2_break = 0;

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

        // F0 = break code prefix
        if (byte == 0xF0) {
            ps2_break = 1;
            continue;
        }

        // ignore released key code
        if (ps2_break) {
            ps2_break = 0;
            continue;
        }

        // A = left turn, D = right turn
        if (byte == 0x1C) {          // A
            pending_turn = -1;
        } else if (byte == 0x23) {   // D
            pending_turn = +1;
        }
    }
}

void handler(void) {
    int mcause_value;
    __asm__ volatile ("csrr %0, mcause" : "=r"(mcause_value));

    // PS/2 IRQ 22 -> mcause 0x80000016
    if (mcause_value == 0x80000016) {
        ps2_isr();
    }
}

void ps2_init(void) {
    volatile int *PS2_ptr = (int *)PS2_BASE;
    int data;
    int mstatus_value = 0b1000;
    int mtvec_value = (int)&handler;
    int mie_value;

    // clear old bytes
    while (1) {
        data = *PS2_ptr;
        if ((data & 0x8000) == 0)
            break;
    }

    // enable PS/2 receive interrupt at device
    *(PS2_ptr + 1) = 0x1;

    // set trap vector and enable IRQ 22
    __asm__ volatile ("csrc mstatus, %0" :: "r"(mstatus_value));
    __asm__ volatile ("csrw mtvec, %0" :: "r"(mtvec_value));

    __asm__ volatile ("csrr %0, mie" : "=r"(mie_value));
    __asm__ volatile ("csrc mie, %0" :: "r"(mie_value));

    mie_value = (1 << 22);
    __asm__ volatile ("csrs mie, %0" :: "r"(mie_value));

    __asm__ volatile ("csrs mstatus, %0" :: "r"(mstatus_value));
}

void ps2_apply_turn(void) {
    if (pending_turn == -1) {
        snake_dir = (snake_dir + 3) % 4;   // left turn
    } else if (pending_turn == +1) {
        snake_dir = (snake_dir + 1) % 4;   // right turn
    }

    pending_turn = 0;
}