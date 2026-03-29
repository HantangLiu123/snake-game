#ifndef GAME_H
#define GAME_H

#define WIDTH 17
#define HEIGHT 15
#define SNAKE_MAX_LENGTH 255

typedef struct
{
    int x;
    int y;
} Coordinate;

typedef enum
{
    STATUS_END = 0,
    STATUS_RUN,
    STATUS_PAUSE
} GameStatus;

typedef enum
{
    DIR_UP,
    DIR_DOWN,
    DIR_LEFT,
    DIR_RIGHT
} Direction;

#endif
