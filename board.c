#include "led-matrix-c.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

struct RGBLedMatrix *matrix;
struct LedCanvas *offscreen_canvas;
int width, height;
int x, y, i;

char octaflip_board[8][8];


static void draw_grid() {
  for (int y = 0; y < 64; ++y) {
    for (int x = 0; x < 64; ++x) {
      if (((x % 7 == 3) || (y % 7 == 3)) && ((x >= 3 && x <= 59) && (y >= 3 && y <= 59))) {
        led_canvas_set_pixel(offscreen_canvas, x, y, 255, 255, 255);
      }
      else led_canvas_set_pixel(offscreen_canvas, x, y, 0, 0, 0);
    }
  }
}


void set_board() {
  struct RGBLedMatrixOptions opts;
  memset(&opts, 0, sizeof(opts));
  opts.rows = 64;
  opts.cols = 64;
  matrix = led_matrix_create_from_options(&opts, NULL, NULL);

  if (matrix == NULL) {
    printf("board error\n");
    exit(1);
  }

  offscreen_canvas = led_matrix_create_offscreen_canvas(matrix);

  led_canvas_get_size(offscreen_canvas, &width, &height);

  draw_grid();

  offscreen_canvas = led_matrix_swap_on_vsync(matrix, offscreen_canvas);
}


void turnoff_board() {
  led_matrix_delete(matrix);
}


void swap_board(char board[8][8]) {
  draw_grid();

  for (int i = 0; i < 8; ++i) {
    for (int j = 0; j < 8; ++j) {
      for (int a = 0; a < 6; ++a) {
        for (int b = 0; b < 6; ++b) {
	  if (board[i][j] == 'R') {
	    if (i > 3) led_canvas_set_pixel(offscreen_canvas,4+7*j+b,4+7*i+a, 0xff, 0, 0);
	    else led_canvas_set_pixel(offscreen_canvas, 4+7*j+b, 4+7*i+a, 0, 0xff, 0);
	  }
	  else if (board[i][j] == 'B') {
	    led_canvas_set_pixel(offscreen_canvas, 4+7*j+b, 4+7*i+a, 0, 0, 0xff);
	  }
	  else if (board[i][j] == '.') {
	    led_canvas_set_pixel(offscreen_canvas, 4+7*j+b, 4+7*i+a, 0, 0, 0);
	  }
	  else {
	    led_canvas_set_pixel(offscreen_canvas, 4+7*j+b, 4+7*i+a, 31, 31, 31);
	  }
	}
      }
    }
  }

  led_matrix_swap_on_vsync(matrix, offscreen_canvas);
}


static void get_inputs() {
  for (int i = 0; i <8; ++i) {
    char input[100];
    fgets(input, sizeof(input), stdin);

    if (sscanf(input, "%c %c %c %c %c %c %c %c",
	       &octaflip_board[i][0], &octaflip_board[i][1],
	       &octaflip_board[i][2], &octaflip_board[i][3],
	       &octaflip_board[i][4], &octaflip_board[i][5],
	       &octaflip_board[i][6], &octaflip_board[i][7]) != 8) {
      printf("Invalid input : 8 inputs required\n");
      exit(1);
    }
    for (int j = 0; j < 8; ++j) {
      if (!(octaflip_board[i][j] == 'R' || octaflip_board[i][j] == 'B' ||
            octaflip_board[i][j] == '.' || octaflip_board[i][j] == '#')) {
        printf("Invalid input : invalid elements\n");
	exit(1);
      }
    }
  }
}


int main(int argc, char* argv[]) {
  set_board();

  get_inputs();

  swap_board(octaflip_board);

  while(1);

  return 0;
}
