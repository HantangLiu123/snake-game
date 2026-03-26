#include "render.h"
#include <stdbool.h>
#include <string.h>
#include "sound.h"

void test_death_hit_wall()
{
    init_snake();

    Coordinate snake_body[SNAKE_MAX_LENGTH] = {{14, 7}, // head（靠近右边界，WIDTH=17的话最大是16）
                                               {13, 7},
                                               {12, 7},
                                               {11, 7},
                                               {-1, -1}};

    // 先画出初始状态
    update_snake(snake_body);

    // 模拟“下一步撞墙”
    Coordinate next_snake_1[SNAKE_MAX_LENGTH] = {{15, 7}, // 头往右 -> 撞墙
                                                 {14, 7},
                                                 {13, 7},
                                                 {12, 7},
                                                 {-1, -1}};
    update_snake(next_snake_1);

    Coordinate next_snake_2[SNAKE_MAX_LENGTH] = {{16, 7}, // 头往右 -> 撞墙
                                                 {15, 7},
                                                 {14, 7},
                                                 {13, 7},
                                                 {-1, -1}};

    update_snake(next_snake_2);

    Coordinate next_snake_3[SNAKE_MAX_LENGTH] = {{17, 7}, // 头往右 -> 撞墙
                                                 {16, 7},
                                                 {15, 7},
                                                 {14, 7},
                                                 {-1, -1}};
    play_gameover_sound();
    update_snake_death(next_snake_3, true);
}

void test_death_hit_self()
{
    init_snake();

    // 一个“U形 + 即将闭合”的形状
    Coordinate snake_body[SNAKE_MAX_LENGTH] = {{8, 5}, // head
                                               {7, 5}, {6, 5}, {6, 6}, {6, 7}, {7, 7}, {8, 7}, {8, 6}, {-1, -1}};

    // 先画当前状态
    update_snake(snake_body);

    // ✅ 下一步：正常向“上”走，但撞到身体 (8,6)
    Coordinate next_snake[SNAKE_MAX_LENGTH] = {{8, 6}, // head 向上走 → 撞到身体
                                               {8, 5}, {7, 5}, {6, 5}, {6, 6}, {6, 7}, {7, 7}, {8, 7}, {-1, -1}};
    play_gameover_sound();
    update_snake_death(next_snake, false);
}

void test_auto_self_collision()
{
    init_snake();
    draw_whole_grid();

    Coordinate snake[SNAKE_MAX_LENGTH];

    // ✅ 更长的初始蛇（关键！）
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

    // ✅ 更紧的回环路径（保证撞）
    int path[][2] = {
        {1, 0},  {1, 0},  // →
        {0, -1}, {0, -1}, // ↑
        {-1, 0}, {-1, 0}, // ←
        {0, 1},  {0, 1}   // ↓（这里一定撞）
    };

    int steps = sizeof(path) / sizeof(path[0]);

    for (int i = 0; i < steps; i++)
    {
        Coordinate next[SNAKE_MAX_LENGTH];

        // 新头
        next[0].x = snake[0].x + path[i][0];
        next[0].y = snake[0].y + path[i][1];

        // 身体跟随
        for (int j = 1; j < length; j++)
        {
            next[j] = snake[j - 1];
        }

        next[length] = (Coordinate){-1, -1};

        // 检测撞自己
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
