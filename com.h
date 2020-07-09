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

#define BUFSIZE 1024
#define SOCKFILE "/tmp/dwm.com.sock"
void printerr(char* msg, int err_no);

// -----------------------------------------------------------------------------
#ifdef _COM_SERVER

#define RET_RESPONSE(msg) memcpy(sendbuf, msg, sizeof(msg)); return sizeof(msg)

int handle_msg(char* msg, unsigned len, char* sendbuf) {
  /* CUSTOM USER CODE HERE */
  printf("Received message: \033[1m%s\033[0m\n", msg);
  if (msg[0] == 'b') {
    togglebar(NULL);
  } else if (msg[0] == 'c') {
    unsigned col_len = 7;
    char* color_ptrs[] = {
      norm_fg, norm_bg, norm_bor,
      sel_fg, sel_bg, sel_bor
    };
    if (len != LENGTH(color_ptrs) * col_len + 2) {
      RET_RESPONSE("Invalid Command");
    }
    for (unsigned i = 0; i < LENGTH(color_ptrs); ++i) {
      char* start = msg + 1 + i * col_len;
      memcpy(color_ptrs[i], start, col_len);
    }
    for (int i = 0; i < LENGTH(colors); i++)
      scheme[i] = drw_scm_create(drw, colors[i], 3);
  }
  RET_RESPONSE("OK");
}
// -----------------------------------------------------------------------------
int sock_fd = -1;
long save_sock_fd;
struct sockaddr_un addr;
char recvbuf[BUFSIZE];
char sendbuf[BUFSIZE];
// -----------------------------------------------------------------------------
void sock_cleanup(int arg) {
  close(sock_fd);
  remove(SOCKFILE);
  exit(0);
}
// -----------------------------------------------------------------------------
int sock_init() {
  signal(SIGINT, sock_cleanup);
  sock_fd = socket(AF_UNIX, SOCK_STREAM, 0);
  if (sock_fd == -1) {
    printerr("Failed to create socket.\n", errno);
    return -1;
  }

  addr.sun_family = AF_UNIX;
  snprintf(addr.sun_path, sizeof(addr.sun_path), SOCKFILE);
  int con = bind(sock_fd, (struct sockaddr *) &addr, sizeof(addr));
  if (con == -1) {
    printerr("Failed to bind socket.", errno);
    return -1;
  }

  int lis = listen(sock_fd, 10);
  if (lis == -1) {
    printerr("Failed to listen on socket.", errno);
    return -1;
  }

  save_sock_fd = fcntl(sock_fd, F_GETFL) | O_NONBLOCK;
  fcntl(sock_fd, F_SETFL, save_sock_fd);

  return sock_fd;
}
// -----------------------------------------------------------------------------
int sock_poll() {
  if (sock_fd == -1) {
    printerr("Socket not initialized yet", errno);
    return -1;
  }
  struct sockaddr_un cli_addr;
  unsigned cli_addr_len;
  int rdaddr = accept(sock_fd, (struct sockaddr *) &cli_addr, &cli_addr_len);
  if (rdaddr < 0) {
    int err = errno;
    if (err == EWOULDBLOCK) {
      return 1;
    }
    printerr("Error while reading from socket", errno);
  }
  int nrecv = recv(rdaddr, recvbuf, BUFSIZE - 1, 0);
  if (nrecv == 0) {
    printerr("Received nothing from first batch", errno);
    return -1;
  }
  recvbuf[nrecv] = '\0';
  int nansw = handle_msg(recvbuf, nrecv, sendbuf);
  int nsend = send(rdaddr, sendbuf, nansw, 0);
  if (nsend == -1) {
    printerr("Failed sending answer", errno);
  }
  struct pollfd fds[] = {{rdaddr, POLLIN, 0}};
  while (poll(fds, 1, 0) > 0) {
    if (fds[0].revents & POLLERR || fds[0].revents & POLLNVAL) {
      printerr("Error on socket, closing", errno);
      break;
    } else if(fds[0].revents & POLLIN) {
      int nrecv = recv(rdaddr, recvbuf, BUFSIZE - 1, 0);
      if (nrecv == 0) {
        break;
      }
      recvbuf[nrecv] = '\0';
      int nansw = handle_msg(recvbuf, nrecv, sendbuf);
      int nsend = send(rdaddr, sendbuf, nansw, 0);
      if (nsend == -1) {
        printerr("Failed sending answer", errno);
      }
    }
  }
  close(rdaddr);
  return 0;
}
#endif //_COM_SERVER
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
#ifdef _COM_CLIENT

#endif //_COM_CLIENT
// -----------------------------------------------------------------------------

void printerr(char* msg, int err_no) {
  fprintf(stderr, "Error: %s, errno: %d\n", msg, err_no);
}

#endif //_COM_H_DEFINED
