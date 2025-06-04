#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cjson/cJSON.h"
#include "octaflip_server.h"

#define Red 'R'
#define Blue 'B'
#define Empty '.'
#define Blocked '#'

userInfo users[2];
int playerNum;

static char _board[8][8];
static char player;
static int moveNum = 1000;
static int passNum;
static int turn;


/*
2 => continue game (pass)
1 => continue game (move_ok) or continue registration
0 => end game or start game
-1 => error occured or invalid move (pass)
*/


void switch_player() {
  if (player == Red) {player = Blue;}
  else {player = Red;}
}


void show_board() {
  for (int i = 0; i < 8; i++) {
    for (int j = 0; j < 8; j++) {
      printf("%c", _board[i][j]);
    }
    printf("\n");
  }
}


char get_player() {
  return player;
}


static int check_range(int i) {
  return i >= 1 && i <= 8;
}


int check_player(char user[100]) {
  for (int i = 0; i < playerNum; i++) {
    if (strcmp(users[i].username, user) == 0) {
      if (users[i].player == player) return 1;
      else return -1;
    }
  }
  return -1;
}


static void flip(int i, int j) {
  for (int a = i-1; a < i+2; a++) {
    for (int b = j-1; b < j+2; b++) {
      if (a < 0 || b < 0 || a > 7 || b > 7);
      else {
        if (a == i && b == j);
	else if (_board[a][b] == Empty || _board[a][b] == Blocked || _board[a][b] == player);
	else _board[a][b] = player;
      }
    }
  }
}


static void clone(int i, int j) {
  _board[i][j] = player;
  flip(i, j);
}


static void jump(int i, int j, int k, int l) {
  _board[i][j] = Empty;
  _board[k][l] = player;
  flip(k, l);
}

//valid move => 0, invalid move => 1
static int act(int i, int j, int k, int l) {
  
  int rowDiff = i - k;
  int columnDiff = j - l;

  if (_board[i][j] != player || _board[k][l] != Empty) {return 1;}

  if (rowDiff == -2 || rowDiff == 2) {
    if (columnDiff == -2 || columnDiff == 0 || columnDiff == 2) jump(i, j, k, l);
    else {return 1;}
  }
  else if (rowDiff == -1 || rowDiff == 1) {
    if (columnDiff == -1 || columnDiff == 0 || columnDiff == 1) clone(k, l);
    else {return 1;}
  }
  else if (rowDiff == 0) {
    if (columnDiff == -2 || columnDiff == 2) jump(i, j, k, l);
    else if (columnDiff == -1 || columnDiff == 1) clone(k, l);
    else {return 1;}
  }
  else {return 1;}

  return 0;
}


static void get_scores(int score1, int score2) {
  for (int i = 0; i < 8; i++) {
    for (int j = 0; j < 8; j++) {
      if (_board[i][j] == Red) {score1 += 1;}
      else if (_board[i][j] == Blue) {score2 += 1;}
      else;
    }
  }
}


static cJSON* get_board() {
  cJSON* board = cJSON_CreateArray();
  for (int i = 0; i < 8; i++) {
    cJSON* row = cJSON_CreateArray();

    for (int j = 0; j < 8; j++) {
      char temp[2] = {_board[i][j], '\0'};
      cJSON_AddItemToArray(row, cJSON_CreateString(temp));
    }
    cJSON_AddItemToArray(board, row);
  }
  
  return board;
}


void show_result() {
  int score1 = 0; int score2 = 0;
  get_scores(score1, score2);

  show_board();
  printf("%s : %d, %s : %d\n", users[0].username, score1, users[1].username, score2);
  if (score1 > score2) {printf("Winner : %s\n", users[0].username);}
  else if (score1 < score2) {printf("Winner : %s\n", users[1].username);}
  else {printf("Draw\n");}
}


//return = 1 => valid, return = 0 => invalid
static int is_valid_pass() {
  for (int i = 0; i < 8; i++) {
    for (int j = 0; j < 8; j++) {
      if (_board[i][j] == player) {
        for (int a = -2; a < 3; a++) {
	  for (int b = -2; b < 3; b++) {
	    int temp = a + b;
	    if ((temp == -3 || temp == -1 || temp == 1 || temp == 3) && (a != 0 && b != 0));
	    else if (i + a < 0 || i + a >= 8 || j + b < 0 || j + b >= 8);
	    else {
	      if (_board[i+a][j+b] == Empty) return 0;
	    }
	  }
	}
      }
      else ;
    }
  }
  return 1;
}

//1 => vaild registration, -1 => error occurs (keep registration)
int registration(char buf[1024]) {
  int signal = 1;

  cJSON* root = cJSON_Parse(buf);

  if (root == NULL) {
    signal = -1;
  }
  else {
    cJSON* username = cJSON_GetObjectItemCaseSensitive(root, "username");
    if (!cJSON_IsString(username) || username->valuestring == NULL) {
      signal = -1;
    }
    else {
      strcpy(users[playerNum].username, username->valuestring);

      if (playerNum < 2) {
        if (playerNum == 0) {users[playerNum].player = Red;}
	else {users[playerNum].player = Blue;}
	signal = 1;

      }
      else {signal = -1;}
    }
  }

  return signal;
}


cJSON* send_ack() {
  
  cJSON* payload = cJSON_CreateObject();

  cJSON* type = NULL;
  type = cJSON_CreateString("register_ack");
  cJSON_AddItemToObject(payload, "type", type);

  return payload;
}


cJSON* send_nack(char* msg) {
  
  cJSON* payload = cJSON_CreateObject();

  cJSON* type = NULL;
  cJSON* reason = NULL;
  
  type = cJSON_CreateString("register_nack");
  cJSON_AddItemToObject(payload, "type", type);

  reason = cJSON_CreateString(msg);
  cJSON_AddItemToObject(payload, "reason", reason);

  return payload;
}


static int initiate() {
  turn = 0;

  for (int i = 0; i < 8; i++) {
    for (int j = 0; j < 8; j++) {
      _board[i][j] = '.';
    }
  }

  _board[0][0] = Red;
  _board[0][7] = Blue;
  _board[7][0] = Blue;
  _board[7][7] = Red;


  player = Red;
  return 1;
}

/*
error code
Invalid move => -1
Invalid turn => -2
Invalid pass => -3
 */
int move(char buf[1024]) {
  int r1, c1, r2, c2;
  int signal = 0;

  turn++;

  printf("%s\n", buf);
  cJSON* root = cJSON_Parse(buf);
  if (root == NULL) {
    passNum++;
    signal = -1;
  }
  else {
    cJSON* username = NULL;
    username = cJSON_GetObjectItemCaseSensitive(root, "username");

    if (!cJSON_IsString(username) || username->valuestring == NULL) {
      passNum++;
      signal = -1;
    }
    else {
      if (check_player(username->valuestring) == -1) {
	printf("player %s : not your turn\n", username->valuestring);
        passNum++;
	signal = -1;
      }
      else {
        cJSON* sx = NULL; cJSON* sy = NULL; cJSON* tx = NULL; cJSON* ty = NULL;

	sx = cJSON_GetObjectItemCaseSensitive(root, "sx");
       	sy = cJSON_GetObjectItemCaseSensitive(root, "sy");
	tx = cJSON_GetObjectItemCaseSensitive(root, "tx"); 
	ty = cJSON_GetObjectItemCaseSensitive(root, "ty");

	if (!cJSON_IsNumber(sx) || !cJSON_IsNumber(sy) || !cJSON_IsNumber(tx) || !cJSON_IsNumber(ty)) {
	  passNum++;
	  signal = -1;
	}
	else {
	  r1 = sx->valueint; c1 = sy->valueint; r2 = tx->valueint; c2 = ty->valueint;
	  if (r1 == 0 && c1 == 0 && r2 == 0 && c2 == 0) {
	    if (is_valid_pass()) {
	      passNum++;
	      signal = 2;
	    }
	    else {
	      passNum++;
	      signal = -3;
	    }
	  }
	  else if (check_range(r1) && check_range(c1) && check_range(r2) && check_range(c2)) {
	    if (act(r1 - 1, c1 - 1, r2 - 1, c2 - 1) == 0) {
	      signal = 1;
	    }
	    else passNum = 0;
	  }
	  else {
	    passNum++;
	    signal = -1;
	  }

	}
      }
    }
    
  } 


  if (passNum >= 2 || turn >= moveNum) signal = 0;
  else ;
  return signal;
}


cJSON* game_start(char user1[100], char user2[100]) {

  initiate();

  cJSON* payload = cJSON_CreateObject();

  cJSON* type = NULL;
  cJSON* players = cJSON_CreateArray();
  cJSON* first_player = NULL;
 
  type = cJSON_CreateString("game_start"); 
  cJSON_AddItemToObject(payload, "type", type);

  cJSON_AddItemToArray(players, cJSON_CreateString(user1));
  cJSON_AddItemToArray(players, cJSON_CreateString(user2));

  cJSON_AddItemToObject(payload, "players", players);

  first_player = cJSON_CreateString(user1);
  cJSON_AddItemToObject(payload, "first_player", first_player);

  return payload;
}


cJSON* move_ok(char username[100]) {
  
  cJSON* payload = cJSON_CreateObject();

  cJSON* type = NULL;
  cJSON* board = get_board();
  cJSON* next_player = NULL;

  type = cJSON_CreateString("move_ok");
  cJSON_AddItemToObject(payload, "type", type);
  
  cJSON_AddItemToObject(payload, "board", board);

  next_player = cJSON_CreateString(username);
  cJSON_AddItemToObject(payload, "next_player", next_player);

  return payload;
}


cJSON* move_invalid(char username[100], char _reason[100]) {
  
  cJSON* payload = cJSON_CreateObject();

  cJSON* type = NULL;
  cJSON* board = get_board();
  cJSON* next_player = NULL;
  cJSON* reason = NULL;

  type = cJSON_CreateString("invalid_move");
  cJSON_AddItemToObject(payload, "type", type);

  cJSON_AddItemToObject(payload, "board", board);

  next_player = cJSON_CreateString(username);
  cJSON_AddItemToObject(payload, "next_player", next_player);

  reason = cJSON_CreateString(_reason);
  cJSON_AddItemToObject(payload, "reason", reason);

  return payload;
}


cJSON* move_pass(char user[100]) {
 
  cJSON* payload = cJSON_CreateObject();
  
  cJSON* type = NULL;
  cJSON* board = get_board();
  cJSON* next_player = NULL;

  type = cJSON_CreateString("pass");
  cJSON_AddItemToObject(payload, "type", type);

  cJSON_AddItemToObject(payload, "board", board);
  
  next_player = cJSON_CreateString(user);
  cJSON_AddItemToObject(payload, "next_player", next_player);
  
  return payload;

}


cJSON* your_turn() {
  
  cJSON* payload = cJSON_CreateObject();

  cJSON* type = NULL;
  cJSON* board = get_board();

  type = cJSON_CreateString("your_turn");
  cJSON_AddItemToObject(payload, "type", type);

  cJSON_AddItemToObject(payload, "board", board);

  cJSON_AddNumberToObject(payload, "timeout", 5);

  return payload;
}


cJSON* game_over(char user1[100], char user2[100]) {
  
  cJSON* payload = cJSON_CreateObject();

  cJSON* type = NULL;
  cJSON* scores = cJSON_CreateObject();
  cJSON* user1_score = NULL;
  cJSON* user2_score = NULL;

  type = cJSON_CreateString("game_over");
  type = cJSON_CreateString("game_over");
  cJSON_AddItemToObject(payload, "type", type);

  int score1 = 0;
  int score2 = 0;

  get_scores(score1, score2);
  
  user1_score = cJSON_CreateNumber(score1);
  user2_score = cJSON_CreateNumber(score2);
  cJSON_AddItemToObject(scores, user1, user1_score);
  cJSON_AddItemToObject(scores, user2, user2_score);
  cJSON_AddItemToObject(payload, "scores", scores);


  return payload;
}
