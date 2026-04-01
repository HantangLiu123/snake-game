#include "game.h"
#include "ps2.h"
#include "render.h"
#include "sound.h"

#include <stdbool.h>
#include <stdlib.h>

#define MAX_APPLES 10

// game state / status
static GameStatus status = STATUS_PAUSE;

// snake data (position & length & direction)
static Coordinate snake[SNAKE_MAX_LENGTH];
static int snake_length = 3;

static Direction current_dir = DIR_RIGHT;
static Direction next_dir = DIR_RIGHT;

// apple data (positions of all apples)
static Coordinate apples[MAX_APPLES];
static int apple_count = 0;

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

// initialize a new game
static void reset_game()
{
    for (int i = 0; i < SNAKE_MAX_LENGTH; i++)
        snake[i] = (Coordinate){-1, -1};

    snake_length = 3;

    // initial snake
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

// handle input in each game tick
static void handle_input()
{
    InputCmd cmd;

    while (1)
    {
        cmd = pop_input();
        if (cmd == CMD_NONE)
            break;

        // space toggle game status
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
            return;
        }

        // directions (should only change when running)
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

        // opposite direction and same direction are not valid
        if (!is_opposite(current_dir, new_dir) && current_dir != new_dir)
        {
            next_dir = new_dir;
            return;
        }
    }
}

// spawn an apple at a random location that is not on the snake or
// on current apples
static void try_spawn_apple()
{
    if (rand() % 10 != 0)
        return;

    if (apple_count >= MAX_APPLES)
        return;

    Coordinate empty[WIDTH * HEIGHT];
    int empty_count = 0;

    for (int x = 0; x < WIDTH; x++)
    {
        for (int y = 0; y < HEIGHT; y++)
        {
            if (!is_on_snake(x, y) && !is_on_apple(x, y))
            {
                empty[empty_count++] = (Coordinate){x, y};
            }
        }
    }

    if (empty_count == 0)
        return;

    int idx = rand() % empty_count;

    Coordinate c = empty[idx];
    apples[apple_count++] = c;

    draw_apple(c.x, c.y);
}

// a game tick
void game_tick()
{
    handle_input();

    if (status != STATUS_RUN)
        return;

    try_spawn_apple();

    // update direction
    current_dir = next_dir;

    Coordinate head = snake[0];
    Coordinate new_head = head;

    // update the head
    if (current_dir == DIR_UP)
        new_head.y--;
    else if (current_dir == DIR_DOWN)
        new_head.y++;
    else if (current_dir == DIR_LEFT)
        new_head.x--;
    else
        new_head.x++;

    // check if the snake dies
    bool hit_wall = (new_head.x < 0 || new_head.x >= WIDTH || new_head.y < 0 || new_head.y >= HEIGHT);

    bool hit_self = is_on_snake(new_head.x, new_head.y);

    // move the snake
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
        draw_gameover_img();
        play_gameover_sound();
        return;
    }

    // check if the snake eats an apple (is head on apple?)
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
        // update length
        snake_length++;
        snake[snake_length - 1] = old_tail;

        // remove the apple
        apples[apple_index] = apples[--apple_count];

        // update scoreboard and play an apple sound
        update_digit(snake_length - 3);
        play_apple_sound();
    }

    // render the snake animation to the new position
    update_snake(snake);

    if (snake_length == SNAKE_MAX_LENGTH)
    {
        status = STATUS_END;
        update_status(STATUS_END);
        draw_win_img();
        play_success_sound();
        return;
    }
}

int main()
{
    ps2_init();
    init_vga();
    audio_init();
    draw_apple_side_bar();

    reset_game();

    while (1)
    {
        game_tick();
        audio_tick();
    }
}
