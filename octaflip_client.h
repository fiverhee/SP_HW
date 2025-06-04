#ifndef OCTAFLIP_CLIENT_H
#define OCTAFLIP_CLIENT_H

#include "cjson/cJSON.h"

int game_start(char buf[1024]);
int your_turn(char buf[1024]);
int game_over(char buf[1024]);
cJSON* registeration();
cJSON* move();
cJSON* send_pass();

#endif
