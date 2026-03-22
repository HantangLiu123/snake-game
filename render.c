#include "address_map.h"
#include "game.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LIGHT_GREEN 0x07e0
#define DARK_GREEN 0x6a0
#define LEAF_GREEN 0x640
#define RED 0xf800
#define DARK_RED 0xc000
#define BLUE 0x000f
#define GRID_BASE_X 10
#define GRID_BASE_Y 15
#define GRID_SIZE 15
#define HALF_GRID 7
#define HALF_BODY_WIDTH 4

#define SIDEBAR_X 260
#define SIDEBAR_Y 20

#define DIGIT_SPACING 2
#define APPLE_DIGIT_GAP 6

#define BG_COLOR 0x0000    // 黑色背景（按你实际改）
#define DIGIT_COLOR 0xFFFF // 白色

volatile int pixel_buffer_start; // global variable
short int Buffer1[240][512];     // 240 rows, 512 (320 + padding) columns
short int Buffer2[240][512];

static Coordinate last_snake[SNAKE_MAX_LENGTH];
static Coordinate snake_critical_points[SNAKE_MAX_LENGTH];

static void swap(int *a, int *b)
{
    int temp = *a;
    *a = *b;
    *b = temp;
}

typedef struct
{
    int front_buffer_addr;
    int back_buffer_addr;
    int resolution;
    int status;
} VGA;

volatile VGA *vga_ptr = (VGA *)PIXEL_BUF_CTRL_BASE;

static void plot_pixel(int x, int y, short int line_color)
{
    volatile short int *one_pixel_address;

    // calculate the address and render the color
    one_pixel_address = pixel_buffer_start + (y << 10) + (x << 1);
    *one_pixel_address = line_color;
}

static void plot_pixel_both_buffers(int x, int y, short int color)
{
    volatile short int *pixel1 = pixel_buffer_start + (y << 10) + (x << 1);

    volatile short int *pixel2 = vga_ptr->front_buffer_addr + (y << 10) + (x << 1);

    *pixel1 = color;
    *pixel2 = color;
}

static void clear_screen()
{
    // draw the whole screen black
    for (int x = 0; x < 320; x++)
    {
        for (int y = 0; y < 240; y++)
        {
            plot_pixel(x, y, 0);
        }
    }
}

static void wait_for_sync()
{
    // store a 1 in front buffer and wait for sync signal to be 0 again
    vga_ptr->front_buffer_addr = 1;
    while (vga_ptr->status & 1)
        ;

    // switch to back buffer
    pixel_buffer_start = vga_ptr->back_buffer_addr;
}

void init_vga()
{
    // store the address of the two buffers into the vga regs
    // and clear the two buffers
    vga_ptr->back_buffer_addr = (int)&Buffer1;
    wait_for_sync();
    pixel_buffer_start = vga_ptr->front_buffer_addr;
    clear_screen();
    vga_ptr->back_buffer_addr = (int)&Buffer2;
    pixel_buffer_start = vga_ptr->back_buffer_addr;
    clear_screen();
}

// converts the game coordinate to the pixel coordinate of the
// grid's center
static Coordinate game_to_grid_center(int game_x, int game_y)
{
    Coordinate grid_center;
    grid_center.x = GRID_BASE_X + game_x * GRID_SIZE;
    grid_center.y = GRID_BASE_Y + game_y * GRID_SIZE;
    return grid_center;
}

// draw the grid on the screen
static void draw_grid(int game_x, int game_y)
{
    int color = ((game_x + game_y) % 2) ? LIGHT_GREEN : DARK_GREEN;
    Coordinate grid_center = game_to_grid_center(game_x, game_y);
    for (int dx = -HALF_GRID; dx <= HALF_GRID; dx++)
        for (int dy = -HALF_GRID; dy <= HALF_GRID; dy++)
            plot_pixel(grid_center.x + dx, grid_center.y + dy, color);
}

// draw all 17 * 15 grids on screen
void draw_whole_grid()
{
    for (int game_x = 0; game_x < WIDTH; game_x++)
        for (int game_y = 0; game_y < HEIGHT; game_y++)
            draw_grid(game_x, game_y);

    wait_for_sync();
}

// draw a horizontal line
static void draw_hline(int x1, int x2, int y, short int color)
{
    for (int x = x1; x <= x2; x++)
        plot_pixel_both_buffers(x, y, color);
}

// draw a vertical line
static void draw_vline(int x, int y1, int y2, short int color)
{
    for (int y = y1; y <= y2; y++)
        plot_pixel_both_buffers(x, y, color);
}

// draw a part of the snake body
// this part is a rectangle, so either x1 = x2 or y1 = y2
static void draw_body_part(int x1, int y1, int x2, int y2)
{
    if (x1 == x2)
    {
        if (y1 > y2)
            swap(&y1, &y2);
        for (int dx = -HALF_BODY_WIDTH; dx <= HALF_BODY_WIDTH; dx++)
            draw_vline(x1 + dx, y1 - HALF_BODY_WIDTH, y2 + HALF_BODY_WIDTH, BLUE);
    }
    else
    {
        if (x1 > x2)
            swap(&x1, &x2);
        for (int dy = -HALF_BODY_WIDTH; dy <= HALF_BODY_WIDTH; dy++)
            draw_hline(x1 - HALF_BODY_WIDTH, x2 + HALF_BODY_WIDTH, y1 + dy, BLUE);
    }
}

// initialization for the snake, set the last snake to empty
void init_snake()
{
    last_snake[0] = (Coordinate){-1, -1};
}

// extract the game coordinate of the head, turning points, and tail of the snake
void extract_snake_keypoints(const Coordinate *snake_body, Coordinate *output_points)
{
    int length = 0;

    // find snake length
    while (length < 255 && snake_body[length].x != -1 && snake_body[length].y != -1)
    {
        length++;
    }

    if (length == 0)
    {
        output_points[0] = (Coordinate){-1, -1};
        return;
    }

    int out_index = 0;

    // extract the head
    output_points[out_index++] = snake_body[0];

    // turning points
    for (int i = 1; i < length - 1; i++)
    {
        int dx1 = snake_body[i].x - snake_body[i - 1].x;
        int dy1 = snake_body[i].y - snake_body[i - 1].y;

        int dx2 = snake_body[i + 1].x - snake_body[i].x;
        int dy2 = snake_body[i + 1].y - snake_body[i].y;

        if (dx1 != dx2 || dy1 != dy2)
        {
            output_points[out_index++] = snake_body[i];
        }
    }

    // tail
    if (length > 1)
    {
        output_points[out_index++] = snake_body[length - 1];
    }

    // indication of stop
    if (out_index < SNAKE_MAX_LENGTH)
        output_points[out_index] = (Coordinate){-1, -1};
}

static void draw_head(Coordinate head_pixel, bool is_horizontal)
{
    // draw head
    for (int dx = -HALF_BODY_WIDTH; dx <= HALF_BODY_WIDTH; dx++)
    {
        for (int dy = -HALF_BODY_WIDTH; dy <= HALF_BODY_WIDTH; dy++)
        {
            plot_pixel(head_pixel.x + dx, head_pixel.y + dy, BLUE);
        }
    }

    // draw eyes
    int eye_offset_side = 2; // left and right offset

    if (is_horizontal)
    {
        // horizontally -> left right means up down
        plot_pixel(head_pixel.x, head_pixel.y - eye_offset_side, 0xffff);
        plot_pixel(head_pixel.x, head_pixel.y + eye_offset_side, 0xffff);
    }
    else
    {
        // vertically -> left right means left right
        plot_pixel(head_pixel.x - eye_offset_side, head_pixel.y, 0xffff);
        plot_pixel(head_pixel.x + eye_offset_side, head_pixel.y, 0xffff);
    }
}

static void erase_snake_body(int px, int py)
{
    int gx = (px - GRID_BASE_X + HALF_GRID) / GRID_SIZE;
    int gy = (py - GRID_BASE_Y + HALF_GRID) / GRID_SIZE;

    int color = ((gx + gy) % 2) ? LIGHT_GREEN : DARK_GREEN;

    plot_pixel_both_buffers(px, py, color);
}

// update the snake from its last position to its current position.
// creates the animation of the snake going from its last position
// to the current position.
void update_snake(const Coordinate *snake_body)
{
    // find new length
    int length = 0;
    while (snake_body[length].x != -1)
        length++;

    // first frame
    if (last_snake[0].x == -1)
    {
        extract_snake_keypoints(snake_body, snake_critical_points);

        for (int i = 0; snake_critical_points[i + 1].x != -1; i++)
        {
            Coordinate a = game_to_grid_center(snake_critical_points[i].x, snake_critical_points[i].y);

            Coordinate b = game_to_grid_center(snake_critical_points[i + 1].x, snake_critical_points[i + 1].y);

            draw_body_part(a.x, a.y, b.x, b.y);
        }

        bool is_horizontal = ((snake_critical_points[0].x - snake_critical_points[1].x) != 0);
        Coordinate head_coord = game_to_grid_center(snake_critical_points[0].x, snake_critical_points[0].y);
        draw_head(head_coord, is_horizontal);

        wait_for_sync();

        memcpy(last_snake, snake_body, sizeof(Coordinate) * SNAKE_MAX_LENGTH);

        return;
    }

    // old length
    int last_length = 0;
    while (last_snake[last_length].x != -1)
        last_length++;

    bool grew = (length > last_length);

    // head
    Coordinate head_now = snake_body[0];
    Coordinate head_last = last_snake[0];

    int head_dx = head_now.x - head_last.x;
    int head_dy = head_now.y - head_last.y;

    Coordinate head_pixel = game_to_grid_center(head_last.x, head_last.y);

    // tail
    Coordinate tail_last = last_snake[last_length - 1];
    Coordinate tail_prev = last_snake[last_length - 2];

    int tail_dx = tail_prev.x - tail_last.x;
    int tail_dy = tail_prev.y - tail_last.y;

    Coordinate tail_pixel = game_to_grid_center(tail_last.x, tail_last.y);

    // animation
    for (int step = 0; step < GRID_SIZE; step++)
    {
        // draw head
        head_pixel.x += head_dx;
        head_pixel.y += head_dy;

        bool is_horizontal = (head_dx != 0);
        draw_head(head_pixel, is_horizontal);

        // erase tail gradually
        if (!grew)
        {
            int erase_x = tail_pixel.x - tail_dx * HALF_BODY_WIDTH;
            int erase_y = tail_pixel.y - tail_dy * HALF_BODY_WIDTH;

            if (tail_dx != 0)
            {
                for (int dy = -HALF_BODY_WIDTH; dy <= HALF_BODY_WIDTH; dy++)
                {
                    int px = erase_x;
                    int py = erase_y + dy;

                    erase_snake_body(px, py);
                    if (tail_dx > 0)
                    {
                        erase_snake_body(px - 1, py);
                    }
                    else
                    {
                        erase_snake_body(px + 1, py);
                    }
                }
                erase_snake_body(erase_x, erase_y + HALF_BODY_WIDTH);
            }
            else
            {
                for (int dx = -HALF_BODY_WIDTH; dx <= HALF_BODY_WIDTH; dx++)
                {
                    int px = erase_x + dx;
                    int py = erase_y;

                    erase_snake_body(px, py);
                    if (tail_dy > 0)
                    {
                        erase_snake_body(px, py - 1);
                    }
                    else
                    {
                        erase_snake_body(px, py + 1);
                    }
                }
            }

            tail_pixel.x += tail_dx;
            tail_pixel.y += tail_dy;
        }

        wait_for_sync();
    }

    // save the current position to last position
    memcpy(last_snake, snake_body, sizeof(Coordinate) * SNAKE_MAX_LENGTH);
}

void draw_apple(int game_x, int game_y)
{
    Coordinate c = game_to_grid_center(game_x, game_y);

    for (int dx = -4; dx <= 4; dx++)
    {
        for (int dy = -4; dy <= 4; dy++)
        {
            // a circle with radius of 4
            if (dx * dx + dy * dy <= 16)
            {
                plot_pixel_both_buffers(c.x + dx, c.y + dy, RED);
            }
        }
    }

    // leaf
    plot_pixel_both_buffers(c.x + 1, c.y - 5, LEAF_GREEN);
    plot_pixel_both_buffers(c.x + 2, c.y - 5, LEAF_GREEN);

    // highlight
    plot_pixel_both_buffers(c.x - 2, c.y - 2, DARK_RED);
}

static void draw_dead_eyes(Coordinate head_pixel, bool is_horizontal)
{
    int offset = 2;

    if (is_horizontal)
    {
        // 两个眼睛上下
        int x = head_pixel.x;

        // 上眼
        plot_pixel(x - 1, head_pixel.y - offset - 1, 0xffff);
        plot_pixel(x + 1, head_pixel.y - offset + 1, 0xffff);
        plot_pixel(x - 1, head_pixel.y - offset + 1, 0xffff);
        plot_pixel(x + 1, head_pixel.y - offset - 1, 0xffff);

        // 下眼
        plot_pixel(x - 1, head_pixel.y + offset - 1, 0xffff);
        plot_pixel(x + 1, head_pixel.y + offset + 1, 0xffff);
        plot_pixel(x - 1, head_pixel.y + offset + 1, 0xffff);
        plot_pixel(x + 1, head_pixel.y + offset - 1, 0xffff);
    }
    else
    {
        // 左右眼
        int y = head_pixel.y;

        // 左眼
        plot_pixel(head_pixel.x - offset - 1, y - 1, 0xffff);
        plot_pixel(head_pixel.x - offset + 1, y + 1, 0xffff);
        plot_pixel(head_pixel.x - offset - 1, y + 1, 0xffff);
        plot_pixel(head_pixel.x - offset + 1, y - 1, 0xffff);

        // 右眼
        plot_pixel(head_pixel.x + offset - 1, y - 1, 0xffff);
        plot_pixel(head_pixel.x + offset + 1, y + 1, 0xffff);
        plot_pixel(head_pixel.x + offset - 1, y + 1, 0xffff);
        plot_pixel(head_pixel.x + offset + 1, y - 1, 0xffff);
    }
}

static void erase_cell(Coordinate game_coord)
{
    Coordinate c = game_to_grid_center(game_coord.x, game_coord.y);

    for (int dx = -HALF_GRID; dx <= HALF_GRID; dx++)
    {
        for (int dy = -HALF_GRID; dy <= HALF_GRID; dy++)
        {
            int px = c.x + dx;
            int py = c.y + dy;

            int gx = game_coord.x;
            int gy = game_coord.y;

            int color = ((gx + gy) % 2) ? LIGHT_GREEN : DARK_GREEN;

            plot_pixel_both_buffers(px, py, color);
        }
    }
}

static void flash_snake_death(const Coordinate *death_snake, int length, Coordinate head_pixel, bool is_horizontal)
{
    for (int k = 0; k < 2; k++)
    {
        // 1️⃣ 擦掉整条蛇（按格子）
        for (int i = 0; i < length; i++)
        {
            erase_cell(death_snake[i]);
        }
        wait_for_sync();

        // 2️⃣ 重画 body（用 last_snake 结构）
        extract_snake_keypoints(death_snake, snake_critical_points);

        for (int i = 0; snake_critical_points[i + 1].x != -1; i++)
        {
            Coordinate a = game_to_grid_center(snake_critical_points[i].x, snake_critical_points[i].y);

            Coordinate b = game_to_grid_center(snake_critical_points[i + 1].x, snake_critical_points[i + 1].y);

            draw_body_part(a.x, a.y, b.x, b.y);
        }

        // 3️⃣ ❗关键：head 用 pixel 精确位置
        draw_head(head_pixel, is_horizontal);
        draw_dead_eyes(head_pixel, is_horizontal);

        wait_for_sync();
    }
}

static void dissolve_snake(const Coordinate *last_snake, int length)
{
    for (int i = length - 1; i >= 0; i--)
    {
        erase_cell(last_snake[i]);
        wait_for_sync();
    }
}

void update_snake_death(const Coordinate *snake_body, bool hit_on_wall)
{
    // 当前长度
    int length = 0;
    while (snake_body[length].x != -1)
        length++;

    // 上一帧长度
    int last_length = 0;
    while (last_snake[last_length].x != -1)
        last_length++;

    // head movement
    Coordinate head_now = snake_body[0];
    Coordinate head_last = last_snake[0];

    int head_dx = head_now.x - head_last.x;
    int head_dy = head_now.y - head_last.y;

    Coordinate head_pixel = game_to_grid_center(head_last.x, head_last.y);

    // ✅ 根据碰撞类型决定步数
    int steps;
    if (hit_on_wall)
    {
        steps = HALF_GRID - HALF_BODY_WIDTH; // ≈ 3
    }
    else
    {
        steps = GRID_SIZE - 2 * HALF_BODY_WIDTH; // ≈ 6~7
    }

    bool is_horizontal = (head_dx != 0);

    // 动画
    for (int step = 0; step < steps; step++)
    {
        head_pixel.x += head_dx;
        head_pixel.y += head_dy;

        draw_head(head_pixel, is_horizontal);

        wait_for_sync();
    }

    // ✅ 最终停在碰撞点
    draw_head(head_pixel, is_horizontal);

    // ✅ 画叉眼（覆盖）
    draw_dead_eyes(head_pixel, is_horizontal);

    wait_for_sync();

    // ✨ 1. 闪烁
    flash_snake_death(last_snake, length, head_pixel, is_horizontal);

    // ✨ 2. 逐段消失
    dissolve_snake(last_snake, length);

    memcpy(last_snake, snake_body, sizeof(Coordinate) * SNAKE_MAX_LENGTH);
}

#define APPLE_HEIGHT 16
#define APPLE_WIDTH 16

// array size is 512
static const unsigned short int apple[] = {
    0xf7be, 0xf7be, 0xf7be, 0xf7be, 0xf7be, 0xf7be, 0xf7be, 0xef5d, 0xf79e, 0xe75d, 0xb5f6, 0xb5f6, 0xdefb, 0xf79e,
    0xf7be, 0xf7be, 0xf7be, 0xf7be, 0xf7be, 0xf7be, 0xf7be, 0xf7be, 0xa554, 0x2aa6, 0x9d53, 0x440a, 0x0446, 0x0c46,
    0x0b85, 0x4c0b, 0x9d53, 0xe73c, 0xf7be, 0xf7be, 0xef7d, 0xd69a, 0xb534, 0xac51, 0x9c0f, 0x1aa4, 0x2244, 0x0b24,
    0x03a4, 0x03a4, 0x0324, 0x2b27, 0xb5d6, 0xf79e, 0xf7be, 0xdedb, 0x8b6d, 0x9082, 0xf041, 0xf041, 0xd841, 0x4921,
    0xa0c0, 0xc820, 0x9840, 0xa040, 0x8040, 0x834c, 0xdeba, 0xf7be, 0xe71c, 0x8b2c, 0xb861, 0xf000, 0xf800, 0xf800,
    0xc800, 0x8880, 0xa860, 0xd800, 0xf800, 0xf800, 0xf081, 0xc882, 0x938e, 0xe73c, 0xb534, 0x9082, 0xe800, 0xf800,
    0xf800, 0xf800, 0xf800, 0xe800, 0xe800, 0xf800, 0xf800, 0xf800, 0xf963, 0xf102, 0x98a2, 0xbd75, 0x936d, 0xd841,
    0xf800, 0xf800, 0xf800, 0xf800, 0xf800, 0xf800, 0xf800, 0xf800, 0xf800, 0xf800, 0xf840, 0xf942, 0xe8c2, 0x9b8e,
    0x92eb, 0xe020, 0xf800, 0xf800, 0xf800, 0xf800, 0xf800, 0xf800, 0xf800, 0xf800, 0xf800, 0xf800, 0xf820, 0xf9a3,
    0xf102, 0x92eb, 0x92eb, 0xe020, 0xf800, 0xf800, 0xf800, 0xf800, 0xf800, 0xf800, 0xf800, 0xf800, 0xf800, 0xf800,
    0xf820, 0xf9a3, 0xf0e2, 0x9aeb, 0x936d, 0xd841, 0xf800, 0xf800, 0xf800, 0xf800, 0xf800, 0xf800, 0xf800, 0xf800,
    0xf800, 0xf800, 0xf820, 0xf983, 0xe8c2, 0x9bae, 0xacd3, 0xa861, 0xf000, 0xf800, 0xf800, 0xf800, 0xf800, 0xf800,
    0xf800, 0xf800, 0xf800, 0xf800, 0xf860, 0xf983, 0xb0c2, 0xb514, 0xdeba, 0x6124, 0xd800, 0xf800, 0xf800, 0xf800,
    0xf800, 0xf800, 0xf800, 0xf800, 0xf800, 0xf800, 0xf8e2, 0xe8e2, 0x6986, 0xdedb, 0xf79e, 0xc596, 0x90c3, 0xe800,
    0xf800, 0xf800, 0xf800, 0xf800, 0xf800, 0xf800, 0xf800, 0xf800, 0xf0c1, 0x9903, 0xcdd7, 0xf79e, 0xf7be, 0xef5d,
    0xb4f3, 0x80a2, 0xe020, 0xe800, 0xf000, 0xf000, 0xf800, 0xf800, 0xf000, 0xf040, 0x80e3, 0xb514, 0xef5d, 0xf7be,
    0xf7be, 0xf7be, 0xef7d, 0xce38, 0x8b2c, 0x8082, 0xa841, 0xa820, 0xa020, 0xa041, 0x7882, 0x8b4d, 0xce59, 0xf79e,
    0xf7be, 0xf7be, 0xf7be, 0xf7be, 0xf7be, 0xf7be, 0xf7be, 0xe71c, 0xc618, 0xc618, 0xc618, 0xc618, 0xe73c, 0xf7be,
    0xf7be, 0xf7be, 0xf7be, 0xf7be};

void draw_apple_side_bar()
{
    for (int i = 0; i < APPLE_HEIGHT; i++)
    {
        for (int j = 0; j < APPLE_WIDTH; j++)
        {
            unsigned short color = apple[i * APPLE_WIDTH + j];

            int r = (color >> 11) & 0x1F;
            int g = (color >> 5) & 0x3F;
            int b = color & 0x1F;

            // 亮度（简单近似）
            int brightness = r + g + b;

            // 判断：亮 + 接近灰白
            if (!(brightness > 80 && abs(r - b) < 5 && abs(r - g / 2) < 5))
            {
                plot_pixel_both_buffers(SIDEBAR_X + j, SIDEBAR_Y + i, color);
            }
        }
    }
}

void clear_apple_side_bar()
{
    for (int i = 0; i < APPLE_HEIGHT; i++)
    {
        for (int j = 0; j < APPLE_WIDTH; j++)
        {
            plot_pixel_both_buffers(SIDEBAR_X + j, SIDEBAR_Y + i, BG_COLOR);
        }
    }
}

#define DIGIT_W 10
#define DIGIT_H 14

const char digits[10][DIGIT_H][DIGIT_W] = {
    // 0
    {{0, 0, 1, 1, 1, 1, 1, 1, 0, 0},
     {0, 1, 1, 0, 0, 0, 0, 1, 1, 0},
     {1, 1, 0, 0, 0, 0, 0, 0, 1, 1},
     {1, 1, 0, 0, 0, 0, 0, 0, 1, 1},
     {1, 1, 0, 0, 1, 1, 0, 0, 1, 1},
     {1, 1, 0, 1, 1, 1, 1, 0, 1, 1},
     {1, 1, 0, 1, 1, 1, 1, 0, 1, 1},
     {1, 1, 0, 0, 1, 1, 0, 0, 1, 1},
     {1, 1, 0, 0, 0, 0, 0, 0, 1, 1},
     {1, 1, 0, 0, 0, 0, 0, 0, 1, 1},
     {1, 1, 0, 0, 0, 0, 0, 0, 1, 1},
     {0, 1, 1, 0, 0, 0, 0, 1, 1, 0},
     {0, 0, 1, 1, 1, 1, 1, 1, 0, 0},
     {0, 0, 0, 0, 0, 0, 0, 0, 0, 0}},

    // 1
    {{0, 0, 0, 1, 1, 0, 0, 0, 0, 0},
     {0, 0, 1, 1, 1, 0, 0, 0, 0, 0},
     {0, 1, 1, 1, 1, 0, 0, 0, 0, 0},
     {0, 0, 0, 1, 1, 0, 0, 0, 0, 0},
     {0, 0, 0, 1, 1, 0, 0, 0, 0, 0},
     {0, 0, 0, 1, 1, 0, 0, 0, 0, 0},
     {0, 0, 0, 1, 1, 0, 0, 0, 0, 0},
     {0, 0, 0, 1, 1, 0, 0, 0, 0, 0},
     {0, 0, 0, 1, 1, 0, 0, 0, 0, 0},
     {0, 0, 0, 1, 1, 0, 0, 0, 0, 0},
     {0, 0, 0, 1, 1, 0, 0, 0, 0, 0},
     {0, 1, 1, 1, 1, 1, 1, 0, 0, 0},
     {0, 1, 1, 1, 1, 1, 1, 0, 0, 0},
     {0, 0, 0, 0, 0, 0, 0, 0, 0, 0}},

    // 2
    {{0, 0, 1, 1, 1, 1, 1, 1, 0, 0},
     {0, 1, 1, 0, 0, 0, 0, 1, 1, 0},
     {1, 1, 0, 0, 0, 0, 0, 0, 1, 1},
     {0, 0, 0, 0, 0, 0, 0, 0, 1, 1},
     {0, 0, 0, 0, 0, 0, 0, 1, 1, 0},
     {0, 0, 0, 0, 0, 1, 1, 1, 0, 0},
     {0, 0, 0, 0, 1, 1, 1, 0, 0, 0},
     {0, 0, 0, 1, 1, 0, 0, 0, 0, 0},
     {0, 0, 1, 1, 0, 0, 0, 0, 0, 0},
     {0, 1, 1, 0, 0, 0, 0, 0, 0, 0},
     {1, 1, 0, 0, 0, 0, 0, 0, 0, 0},
     {1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
     {1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
     {0, 0, 0, 0, 0, 0, 0, 0, 0, 0}},

    // 3
    {{0, 0, 1, 1, 1, 1, 1, 1, 0, 0},
     {0, 1, 1, 0, 0, 0, 0, 1, 1, 0},
     {1, 1, 0, 0, 0, 0, 0, 0, 1, 1},
     {0, 0, 0, 0, 0, 0, 0, 0, 1, 1},
     {0, 0, 0, 0, 1, 1, 1, 1, 0, 0},
     {0, 0, 0, 0, 1, 1, 1, 1, 0, 0},
     {0, 0, 0, 0, 0, 0, 0, 1, 1, 0},
     {0, 0, 0, 0, 0, 0, 0, 0, 1, 1},
     {0, 0, 0, 0, 0, 0, 0, 0, 1, 1},
     {1, 1, 0, 0, 0, 0, 0, 0, 1, 1},
     {0, 1, 1, 0, 0, 0, 0, 1, 1, 0},
     {0, 0, 1, 1, 1, 1, 1, 1, 0, 0},
     {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
     {0, 0, 0, 0, 0, 0, 0, 0, 0, 0}},

    // 4
    {{0, 0, 0, 0, 1, 1, 0, 0, 0, 0},
     {0, 0, 0, 1, 1, 1, 0, 0, 0, 0},
     {0, 0, 1, 1, 1, 1, 0, 0, 0, 0},
     {0, 1, 1, 0, 1, 1, 0, 0, 0, 0},
     {1, 1, 0, 0, 1, 1, 0, 0, 0, 0},
     {1, 1, 0, 0, 1, 1, 0, 0, 0, 0},
     {1, 1, 1, 1, 1, 1, 1, 1, 0, 0},
     {1, 1, 1, 1, 1, 1, 1, 1, 0, 0},
     {0, 0, 0, 0, 1, 1, 0, 0, 0, 0},
     {0, 0, 0, 0, 1, 1, 0, 0, 0, 0},
     {0, 0, 0, 0, 1, 1, 0, 0, 0, 0},
     {0, 0, 0, 0, 1, 1, 0, 0, 0, 0},
     {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
     {0, 0, 0, 0, 0, 0, 0, 0, 0, 0}},

    // 5
    {{1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
     {1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
     {1, 1, 0, 0, 0, 0, 0, 0, 0, 0},
     {1, 1, 0, 0, 0, 0, 0, 0, 0, 0},
     {1, 1, 1, 1, 1, 1, 1, 1, 0, 0},
     {1, 1, 1, 1, 1, 1, 1, 1, 1, 0},
     {0, 0, 0, 0, 0, 0, 0, 1, 1, 0},
     {0, 0, 0, 0, 0, 0, 0, 0, 1, 1},
     {0, 0, 0, 0, 0, 0, 0, 0, 1, 1},
     {1, 1, 0, 0, 0, 0, 0, 0, 1, 1},
     {0, 1, 1, 0, 0, 0, 0, 1, 1, 0},
     {0, 0, 1, 1, 1, 1, 1, 1, 0, 0},
     {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
     {0, 0, 0, 0, 0, 0, 0, 0, 0, 0}},

    // 6
    {{0, 0, 1, 1, 1, 1, 1, 1, 0, 0},
     {0, 1, 1, 0, 0, 0, 0, 1, 1, 0},
     {1, 1, 0, 0, 0, 0, 0, 0, 0, 0},
     {1, 1, 0, 0, 0, 0, 0, 0, 0, 0},
     {1, 1, 1, 1, 1, 1, 1, 1, 0, 0},
     {1, 1, 1, 1, 1, 1, 1, 1, 1, 0},
     {1, 1, 0, 0, 0, 0, 0, 1, 1, 0},
     {1, 1, 0, 0, 0, 0, 0, 0, 1, 1},
     {1, 1, 0, 0, 0, 0, 0, 0, 1, 1},
     {1, 1, 0, 0, 0, 0, 0, 0, 1, 1},
     {0, 1, 1, 0, 0, 0, 0, 1, 1, 0},
     {0, 0, 1, 1, 1, 1, 1, 1, 0, 0},
     {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
     {0, 0, 0, 0, 0, 0, 0, 0, 0, 0}},

    // 7
    {{1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
     {1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
     {0, 0, 0, 0, 0, 0, 0, 1, 1, 0},
     {0, 0, 0, 0, 0, 0, 1, 1, 0, 0},
     {0, 0, 0, 0, 0, 1, 1, 0, 0, 0},
     {0, 0, 0, 0, 1, 1, 0, 0, 0, 0},
     {0, 0, 0, 1, 1, 0, 0, 0, 0, 0},
     {0, 0, 1, 1, 0, 0, 0, 0, 0, 0},
     {0, 1, 1, 0, 0, 0, 0, 0, 0, 0},
     {1, 1, 0, 0, 0, 0, 0, 0, 0, 0},
     {1, 1, 0, 0, 0, 0, 0, 0, 0, 0},
     {1, 1, 0, 0, 0, 0, 0, 0, 0, 0},
     {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
     {0, 0, 0, 0, 0, 0, 0, 0, 0, 0}},

    // 8
    {{0, 0, 1, 1, 1, 1, 1, 1, 0, 0},
     {0, 1, 1, 0, 0, 0, 0, 1, 1, 0},
     {1, 1, 0, 0, 0, 0, 0, 0, 1, 1},
     {1, 1, 0, 0, 0, 0, 0, 0, 1, 1},
     {0, 1, 1, 0, 0, 0, 0, 1, 1, 0},
     {0, 0, 1, 1, 1, 1, 1, 1, 0, 0},
     {0, 1, 1, 0, 0, 0, 0, 1, 1, 0},
     {1, 1, 0, 0, 0, 0, 0, 0, 1, 1},
     {1, 1, 0, 0, 0, 0, 0, 0, 1, 1},
     {1, 1, 0, 0, 0, 0, 0, 0, 1, 1},
     {0, 1, 1, 0, 0, 0, 0, 1, 1, 0},
     {0, 0, 1, 1, 1, 1, 1, 1, 0, 0},
     {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
     {0, 0, 0, 0, 0, 0, 0, 0, 0, 0}},

    // 9
    {{0, 0, 1, 1, 1, 1, 1, 1, 0, 0},
     {0, 1, 1, 0, 0, 0, 0, 1, 1, 0},
     {1, 1, 0, 0, 0, 0, 0, 0, 1, 1},
     {1, 1, 0, 0, 0, 0, 0, 0, 1, 1},
     {1, 1, 0, 0, 0, 0, 0, 0, 1, 1},
     {0, 1, 1, 1, 1, 1, 1, 1, 1, 1},
     {0, 0, 1, 1, 1, 1, 1, 1, 1, 1},
     {0, 0, 0, 0, 0, 0, 0, 0, 1, 1},
     {0, 0, 0, 0, 0, 0, 0, 0, 1, 1},
     {0, 0, 0, 0, 0, 0, 0, 0, 1, 1},
     {0, 1, 1, 0, 0, 0, 0, 1, 1, 0},
     {0, 0, 1, 1, 1, 1, 1, 1, 0, 0},
     {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
     {0, 0, 0, 0, 0, 0, 0, 0, 0, 0}}};

static void draw_digit(int x, int y, int d)
{
    for (int i = 0; i < DIGIT_H; i++)
    {
        for (int j = 0; j < DIGIT_W; j++)
        {
            if (digits[d][i][j])
            {
                plot_pixel_both_buffers(x + j, y + i, DIGIT_COLOR);
            }
        }
    }
}

void clear_digit()
{
    int start_x = SIDEBAR_X + APPLE_WIDTH + APPLE_DIGIT_GAP;
    int start_y = SIDEBAR_Y + (APPLE_HEIGHT - DIGIT_H) / 2;

    int total_width = 3 * DIGIT_W + 2 * DIGIT_SPACING;

    for (int i = 0; i < DIGIT_H; i++)
    {
        for (int j = 0; j < total_width; j++)
        {
            plot_pixel_both_buffers(start_x + j, start_y + i, BG_COLOR);
        }
    }
}

void update_digit(int num)
{
    // 1. 先清掉旧的
    clear_digit();

    // 2. 转字符串
    char buf[4]; // 最大999
    sprintf(buf, "%d", num);

    int start_x = SIDEBAR_X + APPLE_WIDTH + APPLE_DIGIT_GAP;
    int start_y = SIDEBAR_Y + (APPLE_HEIGHT - DIGIT_H) / 2;

    int offset = 0;

    // 3. 逐位画
    for (int i = 0; buf[i] != '\0'; i++)
    {
        int d = buf[i] - '0';
        draw_digit(start_x + offset, start_y, d);
        offset += DIGIT_W + DIGIT_SPACING;
    }
}
