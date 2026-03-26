#include "render.h"
#include <stdbool.h>
#include <string.h>
#include "sound.h"

void test_death_hit_wall()
{
    init_snake();

    Coordinate snake_body[SNAKE_MAX_LENGTH] = {{14, 7}, // head close to right bound
                                               {13, 7},
                                               {12, 7},
                                               {11, 7},
                                               {-1, -1}};

    // draw initial snake
    update_snake(snake_body);

    // going right to the wall
    Coordinate next_snake_1[SNAKE_MAX_LENGTH] = {{15, 7},
                                                 {14, 7},
                                                 {13, 7},
                                                 {12, 7},
                                                 {-1, -1}};
    update_snake(next_snake_1);

    Coordinate next_snake_2[SNAKE_MAX_LENGTH] = {{16, 7},
                                                 {15, 7},
                                                 {14, 7},
                                                 {13, 7},
                                                 {-1, -1}};

    update_snake(next_snake_2);

    Coordinate next_snake_3[SNAKE_MAX_LENGTH] = {{17, 7},
                                                 {16, 7},
                                                 {15, 7},
                                                 {14, 7},
                                                 {-1, -1}};
    play_gameover_sound();
    update_snake_death(next_snake_3, true);
}

void test_auto_self_collision()
{
    init_snake();
    draw_whole_grid();

    Coordinate snake[SNAKE_MAX_LENGTH];

    int length = 10;

    snake[0] = (Coordinate){5, 5};
    snake[1] = (Coordinate){4, 5};
    snake[2] = (Coordinate){3, 5};
    snake[3] = (Coordinate){2, 5};
    snake[4] = (Coordinate){1, 5};
    snake[5] = (Coordinate){0, 5};
    snake[6] = (Coordinate){0, 6};
    snake[7] = (Coordinate){0, 7};
    snake[8] = (Coordinate){1, 7};
    snake[9] = (Coordinate){2, 7};
    snake[10] = (Coordinate){-1, -1};

    update_snake(snake);

    // a path that will let the snake hit itself
    int path[][2] = {
        {1, 0},  {1, 0},  // →
        {0, -1}, {0, -1}, // ↑
        {-1, 0}, {-1, 0}, // ←
        {0, 1},  {0, 1}   // ↓
    };

    int steps = sizeof(path) / sizeof(path[0]);

    for (int i = 0; i < steps; i++)
    {
        Coordinate next[SNAKE_MAX_LENGTH];

        // update head
        next[0].x = snake[0].x + path[i][0];
        next[0].y = snake[0].y + path[i][1];

        // body following the head
        for (int j = 1; j < length; j++)
        {
            next[j] = snake[j - 1];
        }

        next[length] = (Coordinate){-1, -1};

        // check if the snake hits itself
        bool hit_self = false;
        for (int j = 1; j < length; j++)
        {
            if (next[0].x == snake[j].x && next[0].y == snake[j].y)
            {
                hit_self = true;
                break;
            }
        }

        if (hit_self)
        {
            
            update_snake_death(next, false);
            play_gameover_sound();
            break;
        }
        else
        {
            update_snake(next);
            memcpy(snake, next, sizeof(Coordinate) * SNAKE_MAX_LENGTH);
        }
    }
}

int main(void)
{
    init_vga();
    draw_whole_grid();
    draw_whole_grid();

    test_auto_self_collision();
    while (1)
        ;
}
