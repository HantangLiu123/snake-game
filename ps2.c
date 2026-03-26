#include "ps2.h"              
#include "address_map.h"      

volatile int snake_dir = DIR_RIGHT;   // 当前蛇真正前进的方向，初始为向右
volatile int pending_turn = 0;        // 待处理的转向：-1 表示左转，+1 表示右转，0 表示不转

static volatile int ps2_break = 0;    // 记录是否刚收到 F0（松开键前缀），1 表示下一个字节是 break code

static void ps2_isr(void) {           // PS/2 中断服务函数，只在本文件内部使用
    volatile int *PS2_ptr = (int *)PS2_BASE;   // 定义指向 PS/2 数据寄存器基地址的指针
    int data;                         // 用来保存从 PS/2 端口读出的 32 位数据
    unsigned char byte;               // 用来保存真正的 8 位键盘扫描码

    while (1) {                       // 一直读，直到 FIFO 里没有有效数据为止
        data = *PS2_ptr;              // 从 PS/2 数据寄存器读一次数据

        // bit15 = RVALID
        if ((data & 0x8000) == 0)     // 如果 bit15 不是 1，说明这次读到的数据无效
            break;                    // 跳出循环，不再继续读取

        byte = (unsigned char)(data & 0xFF);   // 取低 8 位，得到真正的键盘扫描码

        // F0 = break code prefix
        if (byte == 0xF0) {           // 如果收到 F0，说明这是“松开键”序列的前缀
            ps2_break = 1;            // 记录下来，表示下一个字节属于松开键
            continue;                 // 本次不处理方向，继续读下一个字节
        }

        // ignore released key code
        if (ps2_break) {              // 如果之前已经收到过 F0
            ps2_break = 0;            // 清除标志
            continue;                 // 忽略这个松开键的实际键值，不作处理
        }

        // A = left turn, D = right turn
        if (byte == 0x1C) {           // 如果扫描码是 0x1C，对应键盘上的 A
            pending_turn = -1;        // 记录为“下一步左转”
        } else if (byte == 0x23) {    // 如果扫描码是 0x23，对应键盘上的 D
            pending_turn = +1;        // 记录为“下一步右转”
        }
    }
}

void handler(void) {                  // 总中断处理函数，CPU 发生中断时会跳到这里
    int mcause_value;                 // 用来保存 mcause 寄存器的值
    __asm__ volatile ("csrr %0, mcause" : "=r"(mcause_value));   // 读取 mcause，判断中断来源

    // PS/2 IRQ 22 -> mcause 0x80000016
    if (mcause_value == 0x80000016) { // 如果是 PS/2 设备产生的中断
        ps2_isr();                    // 调用 PS/2 专用中断服务函数
    }
}

void ps2_init(void) {                 // PS/2 初始化函数，在 main 里调用一次
    volatile int *PS2_ptr = (int *)PS2_BASE;   // 定义指向 PS/2 基地址的指针
    int data;                         // 临时变量，用来清空旧数据
    int mstatus_value = 0b1000;      // mstatus 的 MIE 位，用来开总中断
    int mtvec_value = (int)&handler; // 中断入口地址，设置为 handler 函数地址
    int mie_value;                    // 保存 mie 寄存器内容，用来开具体中断源

    // clear old bytes
    while (1) {                       // 循环把 PS/2 FIFO 里残留的旧按键数据清空
        data = *PS2_ptr;              // 读取一次 PS/2 数据寄存器
        if ((data & 0x8000) == 0)     // 如果没有有效数据了
            break;                    // 就停止清空
    }

    // enable PS/2 receive interrupt at device
    *(PS2_ptr + 1) = 0x1;             // 向 PS/2 控制寄存器写 1，打开设备接收中断

    // set trap vector and enable IRQ 22
    __asm__ volatile ("csrc mstatus, %0" :: "r"(mstatus_value)); // 先清 MIE，暂时关闭总中断
    __asm__ volatile ("csrw mtvec, %0" :: "r"(mtvec_value));     // 设置中断入口地址 mtvec = handler

    __asm__ volatile ("csrr %0, mie" : "=r"(mie_value));         // 读出当前 mie 寄存器
    __asm__ volatile ("csrc mie, %0" :: "r"(mie_value));         // 先把原来开的中断位清掉

    mie_value = (1 << 22);              // 只打开 IRQ 22，对应 PS/2 中断
    __asm__ volatile ("csrs mie, %0" :: "r"(mie_value));         // 把 IRQ 22 写回 mie，开启该中断源

    __asm__ volatile ("csrs mstatus, %0" :: "r"(mstatus_value)); // 最后重新打开总中断
}

void ps2_apply_turn(void) {           // 在每一步蛇移动前调用，用来真正应用转向
    if (pending_turn == -1) {         // 如果待转向是左转
        snake_dir = (snake_dir + 3) % 4;   // 方向逆时针转 90 度（等价于 -1）
    } else if (pending_turn == +1) {  // 如果待转向是右转
        snake_dir = (snake_dir + 1) % 4;   // 方向顺时针转 90 度（等价于 +1）
    }

    pending_turn = 0;                 // 本次转向处理完成后清零，避免重复转向
}