#include "die.h"
#include "buf.h"
#include "loop.h"
#include "socket.h"

#include <stdio.h>
#include <errno.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>

void
tcp_handler(struct loop *loop, int fd, int tag, int type, int flag, void *args)
{
	int len;
	char request[4096];
	char response[] = "HTTP/1.1 200 OK\r\n"
	                  "Server: DemoWebserver\r\n"
	                  "Connection: keep-alive\r\n"
	                  "Content-Type: text/plain\r\n"
                          "Content-Length: 11\r\n\r\n"
	                  "HelloWorld";

	len = recv(fd, request, sizeof(request), 0);
	if (len <= 0) {
		goto err;
	}

	len = send(fd, response, sizeof(response), 0);
	if (len <= 0) {
		goto err;
	}
	return;
err:
	loop_del_event(loop, fd, LOOP_READ | LOOP_WRITE);
	close(fd);
}

void
tcp_acceptor(struct loop *loop, int fd, int tag, int type, int flag, void *args)
{
	int port;
        int acceptfd;
	socket_addr_t addr;
	char addrstr[SOCKET_ADDR_STRLEN];

        acceptfd = socket_accept(fd, &addr);
	socket_set_nonblock(acceptfd);
	socket_addr_str(&addr, addrstr, &port);
	printf("accept: %s:%d on fd %d\n", addrstr, port, acceptfd);

	loop_set_event(loop, acceptfd, 0, LOOP_READ, 0, tcp_handler, NULL);
}

int
main(void)
{
	int fd;
	int err;
	struct loop  *loop;
	socket_addr_t addr;

	signal(SIGPIPE, SIG_IGN);

	loop = loop_open(1024, 1024);
	die_unless(loop != NULL);

	fd = socket_open(AF_INET6, SOCK_STREAM, 0);
	die_unless(fd != -1);

	err = socket_set_nonblock(fd);
	die_unless(err == 0);

	err = socket_set_reuseaddr(fd);
	die_unless(err == 0);

	err = socket_set_reuseport(fd);
	die_unless(err == 0);

	err = socket_set_tcp_nodelay(fd);
	die_unless(err == 0);

	err = socket_set_tcp_nopush(fd);
	die_unless(err == 0);

	err = socket_addr(&addr, "::", 8080);
	die_unless(err == 0);

	err = socket_bind(fd, &addr);
	die_unless(err == 0);

	err = socket_listen(fd, 32);
	die_unless(err == 0);

	err = loop_set_event(loop, fd, 0, LOOP_READ, 0, tcp_acceptor, NULL);
	die_unless(err == 0);

	err = loop_start(loop);
	die_errno_unless(err == 0);

	loop_close(loop);

	return 0;
}
