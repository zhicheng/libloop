#ifndef __SOCKET_H__
#define __SOCKET_H__

#include <sys/socket.h>
#include <netinet/in.h>

#define SOCKET_ADDR_STRLEN	INET6_ADDRSTRLEN

typedef struct {
	socklen_t addrlen;
	union {
		struct sockaddr         sa;
		struct sockaddr_in      si;
		struct sockaddr_in6     s6;
		struct sockaddr_storage ss;
	} addr;
} socket_addr_t;

int
socket_addr(socket_addr_t *addr, char *hostname, int port);

int
socket_addr_str(socket_addr_t *addr, char *addrstr, int *port);

int
socket_open(int domain, int type, int protocol);

int
socket_set_nonblock(int sockfd);

int
socket_set_reuseaddr(int sockfd);

int
socket_set_tcp_nodelay(int sockfd);

int
socket_set_tcp_nopush(int sockfd);

int
socket_set_reuseport(int sockfd);

int
socket_set_pktinfo(int sockfd);

int
socket_bind(int sockfd, socket_addr_t *addr);

int
socket_listen(int sockfd, int backlog);

int
socket_accept(int sockfd, socket_addr_t *addr);

int
socket_connect(int sockfd, socket_addr_t *addr);

ssize_t
socket_recvfromto(int sockfd, void *buf, size_t len, int flags,
	socket_addr_t *src, socket_addr_t *dst);

ssize_t
socket_sendtofrom(int sockfd, void *buf, size_t len, int flags,
	socket_addr_t *dst, socket_addr_t *src);

int
socket_close(int sockfd);

#endif /* __SOCKET_H__ */
