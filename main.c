#include "game.h"
#include "ps2.h"
#include "render.h"

#include <stdbool.h>
#include <stdlib.h>

#define MAX_APPLES 10 // 你可以改

// ================= 状态 =================
static GameStatus status = STATUS_PAUSE;

// ================= 蛇 =================
static Coordinate snake[SNAKE_MAX_LENGTH];
static int snake_length = 3;

static Direction current_dir = DIR_RIGHT;
static Direction next_dir = DIR_RIGHT;

// ================= 苹果 =================
static Coordinate apples[MAX_APPLES];
static int apple_count = 0;

// ================= 工具函数 =================
static bool is_opposite(Direction a, Direction b)
{
    return (a == DIR_UP && b == DIR_DOWN) || (a == DIR_DOWN && b == DIR_UP) || (a == DIR_LEFT && b == DIR_RIGHT) ||
           (a == DIR_RIGHT && b == DIR_LEFT);
}

static bool is_on_snake(int x, int y)
{
    for (int i = 0; i < snake_length; i++)
    {
        if (snake[i].x == x && snake[i].y == y)
            return true;
    }
    return false;
}

static bool is_on_apple(int x, int y)
{
    for (int i = 0; i < apple_count; i++)
    {
        if (apples[i].x == x && apples[i].y == y)
            return true;
    }
    return false;
}

// ================= 初始化 =================
static void reset_game()
{
    for (int i = 0; i < SNAKE_MAX_LENGTH; i++)
        snake[i] = (Coordinate){-1, -1};

    snake_length = 3;

    // 初始蛇：(4,7) 向右
    snake[0] = (Coordinate){4, 7};
    snake[1] = (Coordinate){3, 7};
    snake[2] = (Coordinate){2, 7};

    current_dir = DIR_RIGHT;
    next_dir = DIR_RIGHT;

    apple_count = 0;

    clear_digit();

    update_status(STATUS_PAUSE);

    init_snake();
    draw_whole_grid();
    draw_whole_grid();
    update_snake(snake);
}

// ================= 输入处理 =================
static void handle_input()
{
    InputCmd cmd;

    while (1)
    {
        cmd = pop_input();
        if (cmd == CMD_NONE)
            break;

        // ========= SPACE =========
        if (cmd == CMD_SPACE)
        {
            if (status == STATUS_PAUSE)
            {
                status = STATUS_RUN;
                update_status(STATUS_RUN);
            }
            else if (status == STATUS_RUN)
            {
                status = STATUS_PAUSE;
                update_status(STATUS_PAUSE);
            }
            else if (status == STATUS_END)
            {
                reset_game();
                status = STATUS_PAUSE;
                return;
            }
            return; // 一个 tick 只处理一个有效输入
        }

        // ========= 方向 =========
        if (status != STATUS_RUN)
            continue;

        Direction new_dir;

        if (cmd == CMD_UP)
            new_dir = DIR_UP;
        else if (cmd == CMD_DOWN)
            new_dir = DIR_DOWN;
        else if (cmd == CMD_LEFT)
            new_dir = DIR_LEFT;
        else
            new_dir = DIR_RIGHT;

        // 防止反向
        if (!is_opposite(current_dir, new_dir) && current_dir != new_dir)
        {
            next_dir = new_dir;
            return;
        }
    }
}

// ================= 随机生成苹果 =================
static void try_spawn_apple()
{
    // 随机概率生成
    if (rand() % 10 != 0)
        return;

    if (apple_count >= MAX_APPLES)
        return;

    int x, y;

    do
    {
        x = rand() % WIDTH;
        y = rand() % HEIGHT;
    } while (is_on_snake(x, y) || is_on_apple(x, y));

    apples[apple_count++] = (Coordinate){x, y};

    draw_apple(x, y);
}

// ================= 主更新 =================
void game_tick()
{
    handle_input();

    if (status != STATUS_RUN)
        return;

    try_spawn_apple();

    // 更新方向
    current_dir = next_dir;

    Coordinate head = snake[0];
    Coordinate new_head = head;

    // ===== 计算下一步 =====
    if (current_dir == DIR_UP)
        new_head.y--;
    else if (current_dir == DIR_DOWN)
        new_head.y++;
    else if (current_dir == DIR_LEFT)
        new_head.x--;
    else
        new_head.x++;

    // ===== 判断死亡 =====
    bool hit_wall = (new_head.x < 0 || new_head.x >= WIDTH || new_head.y < 0 || new_head.y >= HEIGHT);

    bool hit_self = is_on_snake(new_head.x, new_head.y);

    // ===== 移动蛇 =====
    Coordinate old_tail = snake[snake_length - 1];
    for (int i = snake_length - 1; i > 0; i--)
    {
        snake[i] = snake[i - 1];
    }
    snake[0] = new_head;

    if (hit_wall || hit_self)
    {
        update_snake_death(snake, hit_wall);
        status = STATUS_END;
        update_status(STATUS_END);
        return;
    }

    if (snake_length == SNAKE_MAX_LENGTH)
    {
        status = STATUS_END;
        update_status(STATUS_END);
        return;
    }

    // ===== 是否吃苹果 =====
    bool ate = false;
    int apple_index = -1;

    for (int i = 0; i < apple_count; i++)
    {
        if (apples[i].x == new_head.x && apples[i].y == new_head.y)
        {
            ate = true;
            apple_index = i;
            break;
        }
    }

    if (ate)
    {
        snake_length++;
        snake[snake_length - 1] = old_tail;

        // 删除苹果
        apples[apple_index] = apples[--apple_count];

        update_digit(snake_length - 3);
    }

    // ===== 渲染 =====
    update_snake(snake);
}

int main()
{
    ps2_init();
    init_vga();
    draw_apple_side_bar();

    reset_game();

    while (1)
    {
        game_tick();

        // 这里你可以加 delay / timer 控制 tick 速度
    }
}
