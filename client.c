#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <signal.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <setjmp.h>
#include "cjson/cJSON.h"
#include "octaflip_client.h"


int client_fd;
struct sockaddr_in client_addr, server_addr;
socklen_t addr_len = sizeof(client_addr);

jmp_buf env;
int timeout_occurred;

char* server_ip = NULL;
char* username = NULL;
int server_port = -1;


void send_payload(int fd, cJSON* json_msg) {
  char *json = cJSON_PrintUnformatted(json_msg);
  send(client_fd, json, strlen(json)+1, 0);
  send(client_fd, "\n", 1, 0);

}


void get_type(char buf[1024], char type[100]) {
  cJSON* root = cJSON_Parse(buf);

  cJSON* temp = cJSON_GetObjectItemCaseSensitive(root, "type");
  if(!cJSON_IsString(temp) || temp->valuestring == NULL) {
    printf("get_type error\n");
    strcpy(type, "error");
  }
  else {
    strcpy(type, temp->valuestring);
  }

}


//memset after use it
size_t receive(int fd, char buffer[1024]) {
  size_t len = 0;
  char* p;

  len += recv(fd, buffer + len, 1024 - len - 1, 0);
  buffer[len] = '\0';
  while (p = strchr(buffer, '\n')) {
    *p = '\0';
  }
  return len;
}


void timeout_handler(int sig) {
  timeout_occurred = 1;
  
  cJSON* payload = send_pass();
  send_payload(client_fd, payload);

  longjmp(env, 1);
}


int main(int argc, char* argv[]) {
  for (int i = 1; i < argc; ++i) {
    if (strcmp(argv[i], "-ip") == 0 && i+1 < argc) {
      server_ip = argv[i+1];
      i++;
    }
    else if (strcmp(argv[i], "-port") == 0 && i+1 < argc) {
      server_port = atoi(argv[i+1]);
      i++;
    }
    else if (strcmp(argv[i], "-username") == 0 && i+1 < argc) {
      username = argv[i+1];
      i++;
    }
    else;
  }

  if (server_ip == NULL || server_port == -1 || username == NULL) {
    printf("argument error\n");
    exit(1);
  }
  
  struct addrinfo hints, *result, *p;
  int status;

  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;

  status = getaddrinfo(NULL, "10000", &hints, &result);
  if (status != 0) {
    printf("getaddrinfo error\n");
    exit(1);
  }

  for (p = result; p != NULL; p = p->ai_next) {
    client_fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
    if (client_fd == -1) continue;

    if (bind(client_fd, p->ai_addr, p->ai_addrlen) == 0) break;
    close(client_fd);
  }

  if (p == NULL) {
    printf("bind error\n");
    exit(1);
  }

  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(server_port);
  inet_pton(AF_INET, server_ip, &server_addr.sin_addr);

  if (connect(client_fd, (struct sockaddr*) &server_addr, addr_len) < 0) {
    printf("connect error\n");
    exit(1);
  }

  cJSON* regis = registeration(username);

  send_payload(client_fd, regis);

  while(1) {
    usleep(100000);
    char buf[1024];
    char type[100];

    receive(client_fd, buf);
    get_type(buf, type);

    printf("%s\n", buf);

    if (strcmp("register_ack", type) == 0);
    else if (strcmp("register_nack", type) == 0) {
      printf("register_nack\n");
      break;
    }
    else if (strcmp("move_ok", type) == 0);
    else if (strcmp("invalid_move", type) == 0) {
      cJSON* temp = cJSON_Parse(buf);
      cJSON* str = cJSON_GetObjectItemCaseSensitive(temp, "reason");
      printf("Invalid move : %s\n", str->valuestring);

    }
    else if (strcmp("pass", type) == 0);
    else if (strcmp("game_start", type) == 0) {
      if (game_start(buf) == -1) {
        printf("game_start error\n");
	exit(1);
      }
    }
    else if (strcmp("your_turn", type) == 0) {
      int timeout;

      your_turn(buf);

      cJSON* root = cJSON_Parse(buf);
      cJSON* temp = cJSON_GetObjectItemCaseSensitive(root, "timeout");

      if (!cJSON_IsNumber(temp)) {
        printf("timeout error\n");
	timeout = 5;
      }
      else timeout = temp->valueint;

      timeout_occurred = 0;

      if (setjmp(env) == 0) {
	alarm(timeout);
        cJSON* payload = move();

	alarm(0);
	if (!timeout_occurred) {
	  send_payload(client_fd, payload);
	}
	else {
	  printf("time out occurred\n");
	}
      }

    }
    else if (strcmp("game_over", type) == 0) {
      break;
    }
    else {
      printf("type error\n");
      break;
    }

    memset(buf, 0, sizeof(buf));
    memset(type, 0, sizeof(type));
  }

  freeaddrinfo(result);
  freeaddrinfo(p);
  close(client_fd);
  return 0;
}
