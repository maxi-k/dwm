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
       die("Buffer Size %d", BUFSIZE);
    if(!strcmp("-v", argv[1]))
       die("dwm-"VERSION);
    if(!strcmp("-h", argv[1]) || !strcmp("--help", argv[1]))
      die("No Args: Send stdin to dwm\nWith Args:\n -s: Print buffer size\n -v: Print dwm version");
  }

  signal(SIGINT, closer);

  fd = socket(AF_UNIX, SOCK_STREAM, 0);
  if (fd == -1) {
    printerr("Failed to create socket");
    return -1;
  }

  struct sockaddr_un addr;
  addr.sun_family = AF_UNIX;
  snprintf(addr.sun_path, sizeof(addr.sun_path), SOCKFILE);

  int con = connect(fd, (struct sockaddr *) &addr, sizeof(addr));
  if (con == -1) {
    printerr("Failed to connect to socket");
    return -1;
  }

  char inbuf[BUFSIZE];
  char recvbuf[BUFSIZE];
  while(1) {
    char *read = fgets(inbuf, BUFSIZE - 1, stdin);
    // fprintf(stderr, "Read: '%c'; res %s", inbuf[0], read);
    // fflush(stderr);
    if (read == NULL || inbuf[0] == '\0' || inbuf[0] == '\n' || inbuf[0] == EOF) {
      break;
    }
    int len = strlen(inbuf);
    int send_res = send(fd, inbuf, len, 0);
    if (send_res == -1) {
      printerr("Failed to send data to the socket");
      continue;
    }
    // TODO: poll
    int nrecv = recv(fd, recvbuf, BUFSIZE - 1, 0);
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
