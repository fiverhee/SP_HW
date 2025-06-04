#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "cjson/cJSON.h"
#include "movement_generator.h"


#define Red 'R'
#define Blue 'B'
#define Empty '.'
#define Blocked '#'


/*
2 => no movement available (pass)
1 => valid move
-1 => error occured (pass)
*/


char _username[100];
char opponent[100];
char player;

char _board[8][8];


static int parsing_error() {
  printf("parsing error\n");
  return -1;
}


void show_board() {
  for (int i = 0; i < 8; i++) {
    for (int j = 0; j < 8; j++) {
      printf("%c", _board[i][j]);
    }
    printf("\n");
  }
}


int game_start(char buf[1024]) {
  int signal = 1;
  cJSON* root = cJSON_Parse(buf);

  if (root == NULL) {
    signal = -1;
  }
  else {
    cJSON* first_player = cJSON_GetObjectItemCaseSensitive(root, "first_player");
    if (!cJSON_IsString(first_player) || first_player->valuestring == NULL) signal = -1;
    else {
      if (strcmp(_username, first_player->valuestring) == 0) player = Red;
      else player = Blue;
    }
  }
  return signal;
}


//1 => vaild move, -1 => error occur (pass)
int your_turn(char buf[1024]) {
  int signal = 1;
  
  cJSON* root = cJSON_Parse(buf);

  if (root == NULL) {
    return -1;
  }

  cJSON* board = cJSON_GetObjectItemCaseSensitive(root, "board");
  if (!cJSON_IsArray(board)) {
    return -1;
  }
  
  for (int i = 0; i < 8; i++) {
    cJSON* row = cJSON_GetArrayItem(board, i);
    if (!cJSON_IsArray(row)) {
      printf("Array error\n");
      signal = -1;
      break;
    }
    for (int j = 0; j < 8; j++) {
      cJSON* cell = cJSON_GetArrayItem(row, j);
      if (!cJSON_IsString(cell) || cell->valuestring == NULL) {
        signal = -1;
	break;
      }
      else if (cell->valuestring[0] != 'R' && cell->valuestring[0] != 'B' &&
	       cell->valuestring[0] != '.' && cell->valuestring[0] != '#') {
        printf("test\n");
	signal = -1;
	break;
      }
      else {
        _board[i][j] = cell->valuestring[0];
      }
    }
  }

  return signal;
}


int game_over(char buf[1024]) {
  int signal = 1;
  return signal;
}


cJSON* registeration(char name[100]) {
  strcpy(_username, name);

  cJSON* payload = cJSON_CreateObject();

  cJSON* type = NULL;
  cJSON* username = NULL;

  type = cJSON_CreateString("register");
  cJSON_AddItemToObject(payload, "type", type);

  username = cJSON_CreateString(_username);
  cJSON_AddItemToObject(payload, "username", username);

  return payload;
}


cJSON* move() {
  movement* _movement = generate_movement(_board, player);

  cJSON* payload = cJSON_CreateObject();

  cJSON* type = cJSON_CreateString("move");
  cJSON_AddItemToObject(payload, "type", type);

  cJSON* username = cJSON_CreateString(_username);
  cJSON_AddItemToObject(payload, "username", username);
  
  cJSON_AddNumberToObject(payload, "sx", _movement->sx);
  cJSON_AddNumberToObject(payload, "sy", _movement->sy);
  cJSON_AddNumberToObject(payload, "tx", _movement->tx);
  cJSON_AddNumberToObject(payload, "ty", _movement->ty);

  return payload;
}

cJSON* send_pass() {
   cJSON* payload = cJSON_CreateObject();

  cJSON* type = cJSON_CreateString("move");
  cJSON_AddItemToObject(payload, "type", type);

  cJSON* username = cJSON_CreateString(_username);
  cJSON_AddItemToObject(payload, "username", username);

  cJSON_AddNumberToObject(payload, "sx", 0);
  cJSON_AddNumberToObject(payload, "sy", 0);
  cJSON_AddNumberToObject(payload, "tx", 0);
  cJSON_AddNumberToObject(payload, "ty", 0);

  return payload;
 
}
