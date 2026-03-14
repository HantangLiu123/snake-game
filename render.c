#include "address_map.h"
#include "game.h"
#include <stdbool.h>
#include <stdlib.h>

#define LIGHT_GREEN 0x07e0
#define DARK_GREEN 0x6a0
#define BLUE 0x000f
#define GRID_BASE_X 10
#define GRID_BASE_Y 15
#define GRID_SIZE 15
#define HALF_GRID 7
#define HALF_BODY_WIDTH 4

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
    pixel_buffer_start = vga_ptr->back_buffer_addr;
}

void init_vga()
{
    vga_ptr->back_buffer_addr = (int)&Buffer1;
    wait_for_sync();
    pixel_buffer_start = vga_ptr->front_buffer_addr;
    clear_screen();
    vga_ptr->back_buffer_addr = (int)&Buffer2;
    pixel_buffer_start = vga_ptr->back_buffer_addr;
    clear_screen();
}

static Coordinate game_to_grid_center(int game_x, int game_y)
{
    Coordinate grid_center;
    grid_center.x = GRID_BASE_X + game_x * GRID_SIZE;
    grid_center.y = GRID_BASE_Y + game_y * GRID_SIZE;
    return grid_center;
}

static void draw_grid(int game_x, int game_y)
{
    int color = ((game_x + game_y) % 2) ? LIGHT_GREEN : DARK_GREEN;
    Coordinate grid_center = game_to_grid_center(game_x, game_y);
    for (int dx = -HALF_GRID; dx <= HALF_GRID; dx++)
        for (int dy = -HALF_GRID; dy <= HALF_GRID; dy++)
            plot_pixel(grid_center.x + dx, grid_center.y + dy, color);
}

void draw_whole_grid()
{
    for (int game_x = 0; game_x < WIDTH; game_x++)
        for (int game_y = 0; game_y < HEIGHT; game_y++)
            draw_grid(game_x, game_y);

    wait_for_sync();
}

static void draw_hline(int x1, int x2, int y, short int color)
{
    for (int x = x1; x <= x2; x++)
        plot_pixel(x, y, color);
}

static void draw_vline(int x, int y1, int y2, short int color)
{
    for (int y = y1; y <= y2; y++)
        plot_pixel(x, y, color);
}

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

void init_snake()
{
    last_snake[0] = (Coordinate){-1, -1};
}

void extract_snake_keypoints(const Coordinate *snake_body, Coordinate *output_points)
{
    int length = 0;

    // 找蛇长度
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

    // 头
    output_points[out_index++] = snake_body[0];

    // 中间点检测拐点
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

    // 尾
    if (length > 1)
    {
        output_points[out_index++] = snake_body[length - 1];
    }

    // 终止符
    output_points[out_index] = (Coordinate){-1, -1};
}

void update_snake(const Coordinate *snake_body)
{
    // ---------- find new length ----------
    int length = 0;
    while (snake_body[length].x != -1)
        length++;

    // ---------- first frame ----------
    if (last_snake[0].x == -1)
    {
        extract_snake_keypoints(snake_body, snake_critical_points);

        for (int i = 0; snake_critical_points[i + 1].x != -1; i++)
        {
            Coordinate a = game_to_grid_center(snake_critical_points[i].x, snake_critical_points[i].y);

            Coordinate b = game_to_grid_center(snake_critical_points[i + 1].x, snake_critical_points[i + 1].y);

            draw_body_part(a.x, a.y, b.x, b.y);
        }

        wait_for_sync();

        memcpy(last_snake, snake_body, sizeof(Coordinate) * SNAKE_MAX_LENGTH);

        return;
    }

    // ---------- old length ----------
    int last_length = 0;
    while (last_snake[last_length].x != -1)
        last_length++;

    bool grew = (length > last_length);

    // ---------- head ----------
    Coordinate head_now = snake_body[0];
    Coordinate head_last = last_snake[0];

    int head_dx = head_now.x - head_last.x;
    int head_dy = head_now.y - head_last.y;

    Coordinate head_pixel = game_to_grid_center(head_last.x, head_last.y);

    // ---------- tail ----------
    Coordinate tail_last = last_snake[last_length - 1];
    Coordinate tail_prev = last_snake[last_length - 2];

    int tail_dx = tail_prev.x - tail_last.x;
    int tail_dy = tail_prev.y - tail_last.y;

    Coordinate tail_pixel = game_to_grid_center(tail_last.x, tail_last.y);

    // ---------- animation ----------
    for (int step = 0; step < GRID_SIZE; step++)
    {
        // ===== draw head =====
        head_pixel.x += head_dx;
        head_pixel.y += head_dy;

        for (int dx = -HALF_BODY_WIDTH; dx <= HALF_BODY_WIDTH; dx++)
        {
            for (int dy = -HALF_BODY_WIDTH; dy <= HALF_BODY_WIDTH; dy++)
            {
                plot_pixel(head_pixel.x + dx, head_pixel.y + dy, BLUE);
            }
        }

        // ===== erase tail gradually =====
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

                    int gx = (px - GRID_BASE_X + HALF_GRID) / GRID_SIZE;
                    int gy = (py - GRID_BASE_Y + HALF_GRID) / GRID_SIZE;

                    int color = ((gx + gy) % 2) ? LIGHT_GREEN : DARK_GREEN;

                    plot_pixel_both_buffers(px, py, color);
                }
            }
            else
            {
                for (int dx = -HALF_BODY_WIDTH; dx <= HALF_BODY_WIDTH; dx++)
                {
                    int px = erase_x + dx;
                    int py = erase_y;

                    int gx = (px - GRID_BASE_X + HALF_GRID) / GRID_SIZE;
                    int gy = (py - GRID_BASE_Y + HALF_GRID) / GRID_SIZE;

                    int color = ((gx + gy) % 2) ? LIGHT_GREEN : DARK_GREEN;

                    plot_pixel_both_buffers(px, py, color);
                }
            }

            tail_pixel.x += tail_dx;
            tail_pixel.y += tail_dy;
        }

        wait_for_sync();
    }

    // ---------- save snake ----------
    memcpy(last_snake, snake_body, sizeof(Coordinate) * SNAKE_MAX_LENGTH);
}
