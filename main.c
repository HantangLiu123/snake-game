#include "sound.h"
#include "render.h"
#include "ps2.h"

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

/*void step_snake_test()
{
    static int dir = 0;

    Coordinate head = snake[0];

    int minx = 2;
    int maxx = 10;
    int miny = 2;
    int maxy = 10;

    // setting a rectangle for snake to move in the test
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

    // update the body (moving one grid in the direction)
    for (int i = TEST_SNAKE_LEN - 1; i > 0; i--)
        snake[i] = snake[i - 1];

    snake[0] = new_head;
}*/
void step_snake_test()
{
    Coordinate head = snake[0];

    ps2_apply_turn();

    Coordinate new_head = head;

    if (snake_dir == DIR_RIGHT)
        new_head.x++;
    else if (snake_dir == DIR_DOWN)
        new_head.y++;
    else if (snake_dir == DIR_LEFT)
        new_head.x--;
    else if (snake_dir == DIR_UP)
        new_head.y--;

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
    ps2_init();

    draw_apple(10, 3);
    draw_apple_side_bar();
    int num = 0;
    update_digit(num);
    int i = 0;
    int status = 0;
    update_status(status);

    while (1)
    {
        update_snake(snake);

        step_snake_test();
        if (i == 5)
        {
            num++;
            update_digit(num);
            status = (status + 1) % 3;
            update_status(status);
        }
        i = (i + 1) % 10;
    }
}
