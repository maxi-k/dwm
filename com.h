#ifndef _COM_H_DEFINED
#define _COM_H_DEFINED

#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <poll.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <X11/Xlib.h>

#define COM_HEADERSIZE 1
#define COM_XEVENT_HEADER 'x'
#define COM_CLIENT_HEADER 'c'
#define COM_BUFSIZE (sizeof(XEvent) + COM_HEADERSIZE > 1024 ? sizeof(XEvent) + COM_HEADERSIZE : 1024)
#define SOCKFILE "/tmp/dwm.com.sock.test"

typedef struct {
  char header[COM_HEADERSIZE];
  XEvent *event;
} com_x_msg;
typedef struct {
  int did_run;
} com_x_res;

void printerr(char* msg);

// -----------------------------------------------------------------------------
#ifdef _COM_SERVER

int handle_msg(char* msg, unsigned len, char* sendbuf) {
  /* CUSTOM USER CODE HERE */
  printf("Received message: \033[1m%s\033[0m\n", msg);
  if (msg[0] == 'b') {
    togglebar(NULL);
  }
  sendbuf[0] = 'O'; sendbuf[1] = 'K';
  return 3;
}
// -----------------------------------------------------------------------------
int fd = -1;
long save_fd;
struct sockaddr_un addr;
char recvbuf[COM_BUFSIZE];
char sendbuf[COM_BUFSIZE];
int handle_x_msg(com_x_msg *msg, unsigned len, com_x_res* sendbuf);
// -----------------------------------------------------------------------------
void sock_cleanup(int arg) {
  close(fd);
  remove(SOCKFILE);
  exit(0);
}
// -----------------------------------------------------------------------------
int sock_init() {
  signal(SIGINT, sock_cleanup);
  fd = socket(AF_UNIX, SOCK_STREAM, 0);
  if (fd == -1) {
    printerr("Failed to create socket.\n");
    return -1;
  }

  addr.sun_family = AF_UNIX;
  snprintf(addr.sun_path, sizeof(addr.sun_path), SOCKFILE);
  int con = bind(fd, (struct sockaddr *) &addr, sizeof(addr));
  if (con == -1) {
    printerr("Failed to bind socket.\n");
    return -1;
  }
  printf("Connected to socket.\n");

  int lis = listen(fd, 10);
  if (lis == -1) {
    printerr("Failed to listen on socket\n");
    return -1;
  }
  printf("Listening on socket");
  fflush(stdout);
  return fd;
}
// -----------------------------------------------------------------------------
int acc = 0;
int sock_poll() {
  if (fd == -1) {
    printerr("Socket not initialized yet");
    return -1;
  }
  struct sockaddr_un cli_addr;
  unsigned cli_addr_len;
  int rdaddr = accept(fd, (struct sockaddr *) &cli_addr, &cli_addr_len);
  ++acc;
  if (rdaddr < 0) {
    int err = errno;
    if (err == EWOULDBLOCK) {
      return 1;
    }
    printerr("Error while reading from socket");
  }
  printf("Accepted connection on socket\n");
  struct pollfd fds[] = {{rdaddr, POLLIN, 0}};

  while (poll(fds, 1, -1) > 0) {
    fflush(stdout);
    if (fds[0].revents & POLLERR || fds[0].revents & POLLNVAL) {
      printerr("Error on socket, closing");
      break;
    } else if(fds[0].revents & POLLIN) {
      int nrecv = recv(rdaddr, recvbuf, COM_BUFSIZE - 1, 0);
      if (nrecv == 0) {
        break;
      }
      recvbuf[nrecv] = '\0';
      int nansw;
      if (recvbuf[0] == COM_CLIENT_HEADER) {
        nansw = handle_msg(recvbuf + COM_HEADERSIZE, nrecv, sendbuf);
      } else {
        nansw = handle_x_msg((com_x_msg*) recvbuf, nrecv, (com_x_res*) sendbuf);
      }
      int nsend = send(rdaddr, sendbuf, nansw, 0);
      if (nsend == -1) {
        printerr("Failed sending answer");
      }
    }
    fflush(stdin);
    fflush(stderr);
  }
  printf("Stopped polling, closing client socket.\n");
  fflush(stdin);
  fflush(stderr);
  close(rdaddr);
  return 0;
}
// -----------------------------------------------------------------------------
unsigned char handled_x_msg = 0;
int handle_x_msg(com_x_msg* msg, unsigned len, com_x_res* sendbuf) {
  XEvent* ev =  msg->event;
  if ((sendbuf->did_run = (handler[ev->type] == NULL)))
    handler[ev->type](ev); /* call handler */
  ++handled_x_msg;
  return 1;
}
#endif //_COM_SERVER
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
#ifdef _COM_CLIENT

#endif //_COM_CLIENT
// -----------------------------------------------------------------------------

int sock_init_client() {
  int fd = socket(AF_UNIX, SOCK_STREAM, 0);
  if (fd == -1) {
    return -1;
  }

  struct sockaddr_un addr;
  addr.sun_family = AF_UNIX;
  snprintf(addr.sun_path, sizeof(addr.sun_path), SOCKFILE);

  int con = connect(fd, (struct sockaddr *) &addr, sizeof(addr));
  if (con == -1) {
    return -2;
  }
  return fd;
}


void printerr(char* msg) {
  int err_no = errno;
  fprintf(stderr, "Error: %s, errno: %d\n", msg, err_no);
  fflush(stderr);
}

#endif //_COM_H_DEFINED
