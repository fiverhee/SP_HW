#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "movement_generator.h"

#define Red 'R'
#define Blue 'B'
#define Empty '.'
#define Blocked '#'


static int check_range(int i) {
  return i >= 0 && i <= 7;
}


static void boardcpy(char boards[8][8], char boardd[8][8]) {
  for (int i = 0; i < 8; i++) {
    for (int j = 0; j < 8; j++) {
      boardd[i][j] = boards[i][j];
    }
  }
}


static void flip(char board[8][8], char player, int i, int j) {
  for (int a = i-1; a < i+2; a++) {
    for (int b = j-1; b < j+2; b++) {
      if (a == i && b == j);
      else if (check_range(a) && check_range(b)) {
        if (board[a][b] == player || board[a][b] == Empty || board[a][b] == Blocked);
	else board[a][b] = player;
      }
      else;
    }
  }
}


static void clone(char board[8][8], char player, int i, int j) {
  board[i][j] = player;
  flip(board, player, i, j);
}


static void jump(char board[8][8], char player, int i, int j, int k, int l) {
  board[i][j] = Empty;
  board[k][l] = player;
  flip(board, player, k, l);
}


//clone => 1, jump => 0, error => -1
static int is_clone(int sx, int sy, int tx, int ty) {
  int rowDiff = sx - tx;
  int columnDiff = sy - ty;

  if (rowDiff == -2 || rowDiff == 2) {
    if (columnDiff == -2 || columnDiff == 0 || columnDiff == 2) return 0;
    else return -1;
  }
  else if (rowDiff == -1 || rowDiff == 1) {
    if (columnDiff == -1 || columnDiff == 0 || columnDiff == 1) return 1;
    else return -1; 
  }
  else if (rowDiff == 0) {
    if (columnDiff == -2 || columnDiff == 2) return 0;
    else if (columnDiff == -1 || columnDiff == 1) return 1;
    else return -1;
  }
  else return -1;
}


static void act(char board[8][8], char player, int i, int j, int k, int l) {
  if (is_clone(i, j, k, l) == 1) clone(board, player, k, l);
  else if (is_clone(i, j, k, l) == 0) jump(board, player, i, j, k, l);
  else ; 
}


//error => -1
static int get_flip_score(char board[8][8], char player, int tx, int ty) {
  if (board[tx][ty] != Empty) return -1;

  int score = 0;

  for (int i = tx - 1;  i < tx + 2; i++) {
    for (int j = ty - 1; j < ty + 2; j++) {
      if (i == tx && j == ty) ;
      else if (check_range(i) && check_range(j)) {
        if (board[i][j] == Empty || board[i][j] == Blocked || board[i][j] == player);
	else score++;
      }
      else ;
    }
  }
  return score;
}


static int get_score_after(char board[8][8], char player, int sx, int sy, int tx, int ty) {
  int score = 0;
  char opponent;
  
  if (player == Red) opponent = Blue;
  else if (player == Blue) opponent = Red;
  else {printf("movement_generator error\n");}

  char test_board[8][8];
  boardcpy(board, test_board);
  act(test_board, player, sx, sy, tx, ty);

  for (int i = 0; i < 8; i++) {
    for (int j = 0; j < 8; j++) {
      if (test_board[i][j] == opponent) {
	for (int k = i-2; k < i+3; k++) {
	  for (int l = j-2; l < j+3; l++) {
	    int temp = (i - k) + (j - l);
	    if (k == i && j == l);
	    else if ((temp == -3 || temp == -1 || temp == 1 || temp == 3) && (k != i && l != j));
	    else if (check_range(k) && check_range(l) && test_board[k][l] == Empty) {
	      int temp_score = 0;
	      temp_score += is_clone(i, j, k, l);
	      temp_score += get_flip_score(test_board, opponent, k, l);

	      if (temp_score >= score) score = temp_score;
	    }
	    else ;
	  }
	}
      }
    }
  }
  return score;
}	


static int get_score(char board[8][8], char player, int sx, int sy, int tx, int ty) {
  int score = 0;
  score += is_clone(sx, sy, tx, ty);
  score += get_flip_score(board, player, tx, ty);
  score -= get_score_after(board, player, sx, sy, tx, ty);
  return score;
}


movement* generate_movement(char board[8][8], char player) {
  movement* _movement = (movement*) malloc(sizeof(movement));
  int sx = 0; int sy = 0; int tx = 0; int ty = 0;
  int score = -128;


  for (int i = 0; i < 8; i++) {
    for (int j = 0; j < 8; j++) {
      if (board[i][j] == player) {
        for (int k = i-2; k < i+3; k++) {
	  for (int l = j-2; l < j+3; l++) {

	    int temp = (i - k) + (j - l);

	    if (k == i && l == j);
	    else if ((temp == -3 || temp == -1 || temp == 1 || temp == 3) && (k != i && l != j));
	    else if (check_range(k) && check_range(l)) {
	      int temp_score = get_score(board, player, i, j, k, l);
	      if (temp_score >= score) {
	        sx = i; sy = j; tx = k; ty = l;
		score = temp_score;
	      }
	    }
	    else;

	  }
	}
      }
      else;
    }
  }
  _movement->sx = sx+1;
  _movement->sy = sy+1;
  _movement->tx = tx+1;
  _movement->ty = ty+1;
  _movement->preference = score;
  return _movement;
}
