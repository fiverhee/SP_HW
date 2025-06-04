#ifndef MOVEMENT_GENERATOR_H
#define MOVEMENT_GENERATOR_H

typedef struct {
  int sx;
  int sy;
  int tx;
  int ty;
  int preference;
} movement;

movement* generate_movement(char board[8][8], char player);

#endif
