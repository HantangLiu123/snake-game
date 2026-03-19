#include "render.h"

#define TEST_SNAKE_LEN 5

Coordinate snake[SNAKE_MAX_LENGTH];

void init_test_snake()
{
    for (int i = 0; i < SNAKE_MAX_LENGTH; i++)
        snake[i] = (Coordinate){-1, -1};

    snake[0] = (Coordinate){2, 2};
    snake[1] = (Coordinate){1, 2};
    snake[2] = (Coordinate){0, 2};
    snake[3] = (Coordinate){0, 1};
    snake[4] = (Coordinate){0, 0};
}

void step_snake_test()
{
    static int dir = 0;

    Coordinate head = snake[0];

    int minx = 2;
    int maxx = 10;
    int miny = 2;
    int maxy = 10;

    // ---- 控制转向 ----
    if (dir == 0 && head.x == maxx)
        dir = 1;
    if (dir == 1 && head.y == maxy)
        dir = 2;
    if (dir == 2 && head.x == minx)
        dir = 3;
    if (dir == 3 && head.y == miny)
        dir = 0;

    Coordinate new_head = head;

    if (dir == 0)
        new_head.x++;
    if (dir == 1)
        new_head.y++;
    if (dir == 2)
        new_head.x--;
    if (dir == 3)
        new_head.y--;

    // ---- 身体后移 ----
    for (int i = TEST_SNAKE_LEN - 1; i > 0; i--)
        snake[i] = snake[i - 1];

    snake[0] = new_head;
}

int main(void)
{
    init_vga();
    draw_whole_grid();
    draw_whole_grid();

    init_snake();

    init_test_snake();
    draw_apple(10, 3);

    while (1)
    {
        update_snake(snake);

        step_snake_test();
    }
}
