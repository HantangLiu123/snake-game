#include "ps2.h"              
#include "address_map.h"     

volatile int snake_dir = DIR_RIGHT;   // The current actual moving direction of the snake, initialized to right
volatile int pending_turn = 0;        // Pending turn request: -1 = turn left, +1 = turn right, 0 = no turn

static volatile int ps2_break = 0;    // Flag indicating whether F0 (break code prefix) was received

static void ps2_isr(void) {           // PS/2 interrupt service routine, used only inside this file
    volatile int *PS2_ptr = (int *)PS2_BASE;   // Pointer to the PS/2 base address
    int data;                         // Stores the 32-bit value read from the PS/2 port
    unsigned char byte;               // Stores the actual 8-bit scan code from the keyboard

    while (1) {                       // Keep reading until there is no more valid data in the FIFO
        data = *PS2_ptr;              // Read one value from the PS/2 data register

        // bit15 = RVALID
        if ((data & 0x8000) == 0)     // If bit15 is 0, the current read is not valid
            break;                    // Exit the loop because there is no more valid data

        byte = (unsigned char)(data & 0xFF);   // Extract the low 8 bits as the keyboard scan code

        // F0 = break code prefix
        if (byte == 0xF0) {           // If the byte is F0, it means a key release sequence starts here
            ps2_break = 1;            // Set the break flag so the next byte will be ignored
            continue;                 // Skip direction handling for this byte
        }

        // ignore released key code
        if (ps2_break) {              // If the previous byte was F0
            ps2_break = 0;            // Clear the break flag
            continue;                 // Ignore this released key code
        }

        // A = left turn, D = right turn
        if (byte == 0x1C) {           // If the scan code is 0x1C, the key pressed is A
            pending_turn = -1;        // Request a left turn for the next movement step
        } else if (byte == 0x23) {    // If the scan code is 0x23, the key pressed is D
            pending_turn = +1;        // Request a right turn for the next movement step
        }
    }
}

void handler(void) {                  // Global interrupt handler called by the CPU on an interrupt
    int mcause_value;                 // Stores the value of the mcause register
    __asm__ volatile ("csrr %0, mcause" : "=r"(mcause_value));   // Read mcause to determine the interrupt source

    // PS/2 IRQ 22 -> mcause 0x80000016
    if (mcause_value == 0x80000016) { // If the interrupt came from the PS/2 port
        ps2_isr();                    // Call the PS/2-specific interrupt service routine
    }
}

void ps2_init(void) {                 // Initialize the PS/2 device and enable its interrupt
    volatile int *PS2_ptr = (int *)PS2_BASE;   // Pointer to the PS/2 base address
    int data;                         // Temporary variable used to clear old FIFO data
    int mstatus_value = 0b1000;      // MIE bit in mstatus, used to enable global interrupts
    int mtvec_value = (int)&handler; // Address of the interrupt handler
    int mie_value;                    // Stores mie register contents for enabling the specific interrupt source

    // clear old bytes
    while (1) {                       // Clear any old bytes left in the PS/2 FIFO
        data = *PS2_ptr;              // Read one value from the PS/2 port
        if ((data & 0x8000) == 0)     // If there is no valid data left
            break;                    // Stop clearing
    }

    // enable PS/2 receive interrupt at device
    *(PS2_ptr + 1) = 0x1;             // Write 1 to the PS/2 control register to enable receive interrupt

    // set trap vector and enable IRQ 22
    __asm__ volatile ("csrc mstatus, %0" :: "r"(mstatus_value)); // Clear MIE first to temporarily disable global interrupts
    __asm__ volatile ("csrw mtvec, %0" :: "r"(mtvec_value));     // Set mtvec to the address of handler()

    __asm__ volatile ("csrr %0, mie" : "=r"(mie_value));         // Read the current mie register
    __asm__ volatile ("csrc mie, %0" :: "r"(mie_value));         // Clear currently enabled interrupt bits

    mie_value = (1 << 22);              // Enable only IRQ 22, which corresponds to the PS/2 interrupt
    __asm__ volatile ("csrs mie, %0" :: "r"(mie_value));         // Set IRQ 22 in mie

    __asm__ volatile ("csrs mstatus, %0" :: "r"(mstatus_value)); // Re-enable global interrupts
}

void ps2_apply_turn(void) {           // Apply the pending turn before moving the snake one step
    if (pending_turn == -1) {         // If the pending turn request is left
        snake_dir = (snake_dir + 3) % 4;   // Rotate direction 90 degrees counterclockwise
    } else if (pending_turn == +1) {  // If the pending turn request is right
        snake_dir = (snake_dir + 1) % 4;   // Rotate direction 90 degrees clockwise
    }

    pending_turn = 0;                 // Clear the turn request after applying it
}