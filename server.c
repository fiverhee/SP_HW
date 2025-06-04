#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <cjson/cJSON.h>
#include <pthread.h>
#include "octaflip_server.h"

#define PORT 12345
#define Red 'R'
#define Blue 'B'

int server_fd, player_fd1, player_fd2;
struct sockaddr_in server_addr, client_addr1, client_addr2;
socklen_t addr_len = sizeof(server_addr);

extern int playerNum;
extern char player;
extern userInfo users[2];


void switch_fd(int fd) {
  if (users[0].fd == fd) fd = users[1].fd;
  else if (users[1].fd == fd) fd = users[0].fd;
  else {
    printf("switch_fd error\n");
    fd = users[0].fd;
  }
}


void switch_username(char username[100]) {
  if (strcmp(users[0].username, username) == 0) {
    strcpy(username, users[1].username);
  }
  else {
    strcpy(username, users[0].username);
  }
}


void send_payload(int fd, cJSON* json_msg) {
  char *json = cJSON_PrintUnformatted(json_msg);
  send(fd, json, strlen(json)+1, 0);
  send (fd, "\n", 1, 0);

}


size_t receive(int fd, char buffer[1024]) {
  size_t len = 0;
  char* p;
  len += recv(fd, buffer+len, 1024 - len - 1, 0);
  buffer[len] = '\0';
  while (p = strchr(buffer, '\n')) {
    *p = '\0';
  }

  return len;
}


void get_type(char buf[1024], char type[100]) {
  cJSON* root = cJSON_Parse(buf);

  cJSON* temp = cJSON_GetObjectItemCaseSensitive(root, "type");

  if ((!cJSON_IsString(temp)) || temp->valuestring == NULL) {
    printf("get_type error\n");
    strcpy(type, "error");
  }
  else {
    strcpy(type, temp->valuestring);
  }
}


void get_username(char buf[1024], char username[100]) {
  cJSON* root = cJSON_Parse(buf);

  cJSON* temp = cJSON_GetObjectItemCaseSensitive(root, "username");
  if ((!cJSON_IsString(temp)) || temp->valuestring == NULL) {
    printf("get_username error\n");
    strcpy(username, "error");
  }
  else {
    strcpy(username, temp->valuestring);
  }
}


void game_set() {
  cJSON* payload1 = game_start(users[0].username, users[1].username);
  send_payload(users[0].fd, payload1);
  usleep(100000);
  send_payload(users[1].fd, payload1);

  usleep(100000);
  cJSON* payload2 = your_turn();
  send_payload(users[0].fd, payload2);
}


void* client_handler(void* fd) {
  int client_fd = *(int*)fd;

  char buf[1024];
  size_t len;

  while((len = receive(client_fd, buf)) > 0) {
    usleep(100000);
    char type[100];
    char username[100];

    get_type(buf, type);
    get_username(buf, username);

    printf("%s\n", buf);

    if (strcmp("register", type) == 0) {
      int signal = registration(buf);
     
      if (signal == 1) {
        cJSON* root = cJSON_Parse(buf);

	if (playerNum == 0) {
	  strcpy(users[playerNum].username, username);
	  users[playerNum].fd = client_fd;
	  users[playerNum].player = Red;

	  cJSON* payload = send_ack();
	  send_payload(client_fd, payload);

	  playerNum++;
	}
	else if (playerNum == 1) {
	  if (strcmp(users[0].username, username) == 0) {
	    cJSON* payload = send_nack("the chosen username is already taken\n");
	    send_payload(client_fd, payload);

	    break;
	  }
	  strcpy(users[playerNum].username, username);
	  users[playerNum].fd = client_fd;
	  users[playerNum].player = Blue;

	  cJSON* payload = send_ack();
	  send_payload(client_fd, payload);

	  playerNum++;

	  game_set();
	}
	else {
	  cJSON* payload = send_nack("game is already in progress\n");
	  send_payload(client_fd, payload);

	  break;
	}
      }
      else {
        cJSON* payload = send_nack("register error\n");
        send_payload(client_fd, payload);

        break;
      }
    }
    else if (strcmp("move", type) == 0) {
      int signal = move(buf);

      if (signal == 0) {
        cJSON* payload = game_over(users[0].username, users[1].username);
	send_payload(player_fd1, payload);
	usleep(100000);
	send_payload(player_fd2, payload);

	//show_result();
	exit(1);
	break;
      }
      else if (signal == 1) {
	switch_player();

        cJSON* payload1 = move_ok(username);
	cJSON* payload2 = your_turn();
	send_payload(client_fd, payload1);

	usleep(100000);
	switch_fd(client_fd);
	send_payload(client_fd, payload2);
      }
      else if (signal == 2) {
	switch_player();
	switch_username(username);

        cJSON* payload1 = move_pass(username);
	cJSON* payload2 = your_turn();
	send_payload(client_fd, payload1);

	usleep(100000);
	switch_fd(client_fd);
	send_payload(client_fd, payload2);

      }
      else {
	 switch_player();
	 switch_username(username);

	cJSON* payload1;
	cJSON* payload2 = your_turn();
        if (signal == -1) {payload1 = move_invalid(username, "Invalid move");}
	else if (signal == -2) {payload1 = move_invalid(username, "Not your turn");}
	else if (signal == -3) {payload1 = move_invalid(username, "Invalid pass");}
	else {payload1 = move_invalid(username,"Unknown error");}

	send_payload(client_fd, payload1);

	usleep(100000);
	switch_fd(client_fd);
	send_payload(client_fd, payload2);
      }
    }
    show_board();
    memset(buf, 0, sizeof(buf));
    usleep(100000);
  }

  close(client_fd);
  playerNum--;
  return NULL;
}


int main(int argc, char* argv[]) {
  
  struct addrinfo hints, *result, *p;
  int status;

  memset(&hints, 0, sizeof(hints));
  hints.ai_family =AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;

  status = getaddrinfo(NULL, "12345", &hints, &result);
  if (status != 0) {
    printf("getaddrinfo error\n");
    exit(1);
  }

  for (p = result; p != NULL; p = p->ai_next) {
    server_fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
    if (server_fd == -1) continue;

    if (bind(server_fd, p->ai_addr, p->ai_addrlen) == 0) break;
    close(server_fd);
  }
  if (p == NULL) {
    printf("bind error\n");
    exit(1);
  }

  if (listen(server_fd, 5) == -1) {
    printf("listen error\n");
    close(server_fd);
    exit(1);
  }
  
  printf("Port : %d\n", PORT);

  while(1) {
    struct sockaddr_in client_addr;

    int* client_fd = malloc(sizeof(int));
    *client_fd = accept(server_fd, (struct sockaddr*) &client_addr, &addr_len);
    if (*client_fd == -1) {
      printf("accept error\n");
      free(client_fd);
      continue;
    }

    pthread_t tid;
    pthread_create(&tid, NULL, client_handler, client_fd);
    pthread_detach(tid);
  }

  freeaddrinfo(result);
  freeaddrinfo(p);
  close(server_fd);
  return 0;  
}
