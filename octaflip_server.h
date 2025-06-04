#ifndef OCTAFLIP_SERVER_H
#define OCTAFLIP_SERVER_H

#include "cjson/cJSON.h"

typedef struct {
  char username[100];
  int fd;
  char player;
} userInfo;


void switch_player();
int registration(char buf[1024]);
cJSON* send_ack();
cJSON* send_nack();
void show_board();
void show_result();
char get_player();
int move(char buf[1024]);
cJSON* game_start(char user1[100], char user2[100]);
cJSON* move_ok(char user[100]);
cJSON* move_invalid(char user[100], char* _reason);
cJSON* move_pass(char user[100]);
cJSON* your_turn();
cJSON* game_over(char user1[100], char user2[100]);

#endif
