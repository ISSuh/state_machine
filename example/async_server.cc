/**
 *
 *  Copyright:  Copyright (c) 2021, ISSuh
 *
 */

#include <sys/socket.h>
#include <sys/unistd.h>
#include <arpa/inet.h>
#include <sys/epoll.h>

#include <vector>

#include "../state_machine.hpp"

#define PORT_NUM 3600
#define MAXLINE 1024

const int32_t EVENT_SIZE = 1024;

enum class AsyncServerState {
  CONNECT,
  READ,
  WRITE,
  CLOSE,

  START = CONNECT,
  DONE = CLOSE
};

struct udata {
  int fd;
  char name[80];
};

int user_fds[1024];

void send_msg(struct epoll_event ev, char* msg);

int main(int argc, char** argv) {
  int32_t server_sock = socket(AF_INET, SOCK_STREAM, 0);
  if (server_sock < 0) {
    return 0;
  }

  sockaddr_in addr_in;
  addr_in.sin_family = AF_INET;
  addr_in.sin_port = htons(PORT_NUM);
  addr_in.sin_addr.s_addr = htonl(INADDR_ANY);

  if (bind(server_sock, reinterpret_cast<sockaddr*>(&addr_in), sizeof(addr_in)) < 0) {
    return 0;
  }

  if (listen(server_sock, 10) < 0) {
    return 0;
  }

  int32_t epoll_fd = epoll_create(EVENT_SIZE);
  if (epoll_fd < 0) {
    return 0;
  }

  epoll_event server_event;
  server_event.events = EPOLLIN;
  server_event.data.fd = server_sock;
  if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_sock, &server_event) < 0) {
    return 0;
  }

  std::vector<epoll_event> events(EVENT_SIZE);
  while (true) {
    int32_t event_count = epoll_wait(epoll_fd, &events[0], EVENT_SIZE, -1);
    if (event_count == -1) {
      break;
    }

    for (int32_t i = 0 ; i < event_count ; ++i) {

    }

    for (i = 0; i < eventn; i++) {
      if (events[i].data.fd == listenfd)  // 듣기 소켓에서 이벤트가 발생함
      {
        memset(buf, 0x00, MAXLINE);
        clilen = sizeof(struct sockaddr);
        clientfd = accept(listenfd, (struct sockaddr*)&clientaddr, &clilen);
        user_fds[clientfd] = 1;  // 연결 처리

        user_data = malloc(sizeof(user_data));
        user_data->fd = clientfd;

        char* tmp = "First insert your nickname :";
        write(user_data->fd, tmp, 29);

        sleep(1);
        read(user_data->fd, user_data->name, sizeof(user_data->name));
        user_data->name[strlen(user_data->name) - 1] = 0;

        printf("Welcome [%s]  \n", user_data->name);

        sleep(1);
        sprintf(buf, "Okay your nickname : %s\n", user_data->name);

        write(user_data->fd, buf, 40);

        ev.events = EPOLLIN;
        ev.data.ptr = user_data;

        epoll_ctl(epollfd, EPOLL_CTL_ADD, clientfd, &ev);
      } else  // 연결 소켓에서 이벤트가 발생함
      {
        user_data = events[i].data.ptr;
        memset(buf, 0x00, MAXLINE);
        readn = read(user_data->fd, buf, MAXLINE);
        if (readn <= 0)  // 읽는 도중 에러 발생
        {
          epoll_ctl(epollfd, EPOLL_CTL_DEL, user_data->fd, events);
          close(user_data->fd);
          user_fds[user_data->fd] = -1;
          free(user_data);
        } else  // 데이터를 읽는다.
        {
          send_msg(events[i], buf);
        }
      }
    }
  }
}

// client가 보낸 메시지를 다른 client들에게 전송한다.
void send_msg(struct epoll_event ev, char* msg) {
  int i;
  char buf[MAXLINE + 24];
  struct udata* user_data;
  user_data = ev.data.ptr;
  for (i = 0; i < 1024; i++) {
    memset(buf, 0x00, MAXLINE + 24);
    sprintf(buf, "%s : %s", user_data->name, msg);
    if ((user_fds[i] == 1)) {
      write(i, buf, MAXLINE + 24);
    }
  }
}
