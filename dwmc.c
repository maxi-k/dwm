#define _COM_CLIENT
#include "com.h"
#include "util.h"

#include <signal.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <poll.h>

int fd;

void closer(int arg) {
  close(fd);
  exit(0);
}

int main(int argc, char** argv) {

  if (argc == 2) {
    if(!strcmp("-s", argv[1]))
      die("%d", COM_BUFSIZE - COM_HEADERSIZE);
    if(!strcmp("-v", argv[1]))
      die("dwm-"VERSION);
    die("No Args: Send stdin to dwm\nWith Args:\n -s: Print maximum message size\n -v: Print dwm version");
  }

  signal(SIGINT, closer);

  fd = sock_init_client();
  if (fd < 0) {
    printerr("Failed to create or connect to socket.");
    return -1;
  }

  char inbuf[COM_BUFSIZE];
  char recvbuf[COM_BUFSIZE];
  inbuf[0] = COM_CLIENT_HEADER;
  while(1) {
    char *read = fgets(inbuf + COM_HEADERSIZE, COM_BUFSIZE - 1, stdin);
    // fprintf(stderr, "Read: '%c'; res %s", inbuf[0], read);
    // fflush(stderr);
    char msg_start = inbuf[COM_HEADERSIZE];
    if (read == NULL || msg_start == '\0' || msg_start == '\n' || msg_start == EOF) {
      break;
    }
    int len = strlen(inbuf);
    int send_res = send(fd, inbuf, len, 0);
    if (send_res == -1) {
      printerr("Failed to send data to the socket");
      continue;
    }
    // TODO: poll
    int nrecv = recv(fd, recvbuf, COM_BUFSIZE - 1, 0);
    if (nrecv > 0) {
      recvbuf[nrecv] = '\0';
      printf("%s\n", recvbuf);
    } else {
      printerr("Received nothing from socket");
    }
  }
  fflush(stdin);
  fflush(stderr);
  closer(0);
}
