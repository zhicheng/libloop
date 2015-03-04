#include "socket.h"

#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>

int
socket_addr(socket_addr_t *addr, char *hostname, int port)
{
        char portstr[15];

        struct addrinfo *res;
        struct addrinfo  hints;

        snprintf(portstr, sizeof(portstr), "%d", port);
        portstr[sizeof(portstr) - 1] = 0;

        memset(&hints, 0, sizeof(hints));

        hints.ai_flags    = AI_PASSIVE;
        hints.ai_socktype = SOCK_STREAM;

        if(getaddrinfo(hostname, portstr, &hints, &res) == -1 || res == NULL) {
                return -1;
        }

	if (res->ai_addrlen > sizeof(addr->addr)) {
		return -1;
	}

	addr->addrlen = res->ai_addrlen;
	memcpy(&addr->addr, res->ai_addr, res->ai_addrlen);

	freeaddrinfo(res);

	return 0;
}

int
socket_addr_str(socket_addr_t *addr, char *addrstr, int *port)
{
	switch (addr->addr.sa.sa_family) {
	case AF_INET:
		*port = ntohs(addr->addr.si.sin_port);
		if (inet_ntop(addr->addr.sa.sa_family,
		              &addr->addr.si.sin_addr,
		              addrstr,
		              SOCKET_ADDR_STRLEN) != NULL)
		{
			return 0;
		}
		break;
	case AF_INET6:
		*port = ntohs(addr->addr.s6.sin6_port);
		if (inet_ntop(addr->addr.sa.sa_family,
		              &addr->addr.s6.sin6_addr,
		              addrstr,
		              SOCKET_ADDR_STRLEN) != NULL)
		{
			return 0;
		}
		break;
	}
	return -1;
}

int
socket_open(int domain, int type, int protocol)
{
        return  socket(domain, type, protocol);
}

int
socket_set_nonblock(int sockfd)
{
	return fcntl(sockfd, F_SETFL, O_NONBLOCK);
}

int
socket_set_reuseaddr(int sockfd)
{
	int reuseaddr;
        reuseaddr = 1;
        return setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &reuseaddr, sizeof(reuseaddr));
}

int
socket_set_reuseport(int sockfd)
{
#ifdef SO_REUSEPORT
	int reuseport;
        reuseport = 1;
        return setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, &reuseport, sizeof(reuseport));
#else
# warning socket_set_reuseport require SO_REUSEPORT
#endif /* SO_REUSEPORT */
	return -1;
}

int
socket_set_tcp_nodelay(int sockfd)
{
	int tcp_nodelay;
	tcp_nodelay = 1;
	return setsockopt(sockfd, SOL_SOCKET, TCP_NODELAY, &tcp_nodelay, sizeof(tcp_nodelay));
}

int
socket_set_tcp_nopush(int sockfd)
{
	int tcp_nopush;
        tcp_nopush = 1;
#ifdef TCP_NOPUSH
        return setsockopt(sockfd, SOL_SOCKET, TCP_NOPUSH, &tcp_nopush, sizeof(tcp_nopush));
#elif defined(TCP_CORK)
        return setsockopt(sockfd, SOL_SOCKET, TCP_CORK, &tcp_nopush, sizeof(tcp_nopush));
#else
# warning socket_set_tcp_nopush require TCP_NOPUSH or TCP_CORK
#endif
}

int
socket_set_pktinfo(int sockfd)
{
	int rc;
	int pktinfo;
	socket_addr_t addr;

	addr.addrlen = sizeof(addr.addr);
	if ((rc = getsockname(sockfd, &addr.addr.sa, &addr.addrlen)) == -1)
		return rc;

	pktinfo = 1;

	switch (addr.addr.sa.sa_family) {
	case AF_INET:
#ifdef IP_RECVDSTADDR
		return setsockopt(sockfd, IPPROTO_IP, IP_RECVDSTADDR, &pktinfo, sizeof(pktinfo));
#elif defined(IP_RECVPKTINFO)
		return setsockopt(sockfd, IPPROTO_IP, IP_RECVPKTINFO, &pktinfo, sizeof(pktinfo));
#elif defined(IP_PKTINFO)
		return setsockopt(sockfd, IPPROTO_IP, IP_PKTINFO, &pktinfo, sizeof(pktinfo));
#else
# warning socket_set_pktinfo require IP_RECVDSTADDR, IP_RECVPKTINFO or IP_PKTINFO
#endif

	case AF_INET6:
#ifdef IPV6_RECVPKTINFO
		return setsockopt(sockfd, IPPROTO_IPV6, IPV6_RECVPKTINFO, &pktinfo, sizeof(pktinfo));
#elif defined(IPV6_PKTINFO)
		return setsockopt(sockfd, IPPROTO_IPV6, IPV6_PKTINFO, &pktinfo, sizeof(pktinfo));
#else
# warning socket_set_pktinfo require IPV6_RECVDSTADDR or IPV6_PKTINFO
#endif

	}
	return -1;
}

int
socket_bind(int sockfd, socket_addr_t *addr)
{
        return bind(sockfd, &addr->addr.sa, addr->addrlen);
}

int
socket_listen(int sockfd, int backlog)
{
	return listen(sockfd, backlog);
}

int
socket_connect(int sockfd, socket_addr_t *addr)
{
        return connect(sockfd, &addr->addr.sa, addr->addrlen);
}

int
socket_accept(int sockfd, socket_addr_t *addr)
{
	addr->addrlen = sizeof(addr->addr);
	return accept(sockfd, &addr->addr.sa, &addr->addrlen);
}

ssize_t
socket_recvfromto(int sockfd, void *buf, size_t len, int flags,
	socket_addr_t *src, socket_addr_t *dst)
{
#ifdef IP_PKTINFO
	struct iovec    iov;
	struct msghdr   msg;
	struct cmsghdr *cmsg;
#ifdef IPV6_PKTINFO
	char cmsgbuf[CMSG_SPACE(sizeof(struct in6_pktinfo))];
#else /* IPV6_PKTINFO */
	char cmsgbuf[CMSG_SPACE(sizeof(struct in_pktinfo))];
#endif /* IPV6_PKTINFO */
	ssize_t recvlen;

	iov.iov_len  = len;
	iov.iov_base = buf;

	src->addrlen = sizeof(src->addr);
	memset(&msg, 0, sizeof(msg));
	msg.msg_name    = &src->addr;
	msg.msg_namelen = src->addrlen;
	msg.msg_iov     = &iov;
	msg.msg_iovlen  = 1;
	msg.msg_control = cmsgbuf;
	msg.msg_controllen = sizeof(cmsgbuf);

	recvlen = recvmsg(sockfd, &msg, flags);
	if (recvlen < 0) {
		return recvlen;
	}

	memset(dst, 0, sizeof(*dst));

	/* local port */
	dst->addrlen = sizeof(dst->addr);
	getsockname(sockfd, &dst->addr.sa, &dst->addrlen);

	for (cmsg = CMSG_FIRSTHDR(&msg); cmsg != NULL; cmsg = CMSG_NXTHDR(&msg, cmsg)) {
		if (cmsg->cmsg_level == IPPROTO_IP && cmsg->cmsg_type == IP_PKTINFO) {
			struct in_pktinfo *pktinfo;

			pktinfo = (struct in_pktinfo *)(CMSG_DATA(cmsg));

			dst->addrlen = sizeof(dst->addr.s6);
			dst->addr.si.sin_family = AF_INET;
#ifdef HAVE_SIN_LEN
			dst->addr.si.sin_len    = sizeof(dst->addr.si);
#endif
			dst->addr.si.sin_addr   = pktinfo->ipi_addr;

			break;
		}
#ifdef IPV6_PKTINFO
		if (cmsg->cmsg_level == IPPROTO_IPV6 && cmsg->cmsg_type == IPV6_PKTINFO) {
			struct in6_pktinfo *pktinfo;

			pktinfo = (struct in6_pktinfo *)(CMSG_DATA(cmsg));

			dst->addrlen = sizeof(dst->addr.s6);
			dst->addr.s6.sin6_family = AF_INET6;
#ifdef HAVE_SIN6_LEN
			dst->addr.s6.sin6_len    = sizeof(dst->addr.s6);
#endif
			dst->addr.s6.sin6_addr   = pktinfo->ipi6_addr;
			if (IN6_IS_ADDR_LINKLOCAL(&pktinfo->ipi6_addr))
				dst->addr.s6.sin6_scope_id = pktinfo->ipi6_ifindex;

			break;
		}
#endif /* IPV6_PKTINFO */
	}
	return recvlen;
#else /* IP_PKTINFO */
	memset(dst, 0, sizeof(*dst));
	return recvfrom(sockfd, buffer, length, flags, &src->addr.sa, src->addrlen);
#endif /* IP_PKTINFO */
}

ssize_t
socket_sendtofrom(int sockfd, void *buf, size_t len, int flags,
	socket_addr_t *dst, socket_addr_t *src)
{
#ifdef IP_PKTINFO
	struct iovec    iov;
	struct msghdr   msg;
	struct cmsghdr *cmsg;
#ifdef IPV6_PKTINFO
	char cmsgbuf[CMSG_SPACE(sizeof(struct in6_pktinfo))];
#else /* IPV6_PKTINFO */
	char cmsgbuf[CMSG_SPACE(sizeof(struct in_pktinfo))];
#endif /* IPV6_PKTINFO */

	iov.iov_len  = len;
	iov.iov_base = buf;

	memset(&msg,     0, sizeof(msg));
	memset(&cmsgbuf, 0, sizeof(cmsgbuf));

	msg.msg_name    = &dst->addr;
#ifdef HAVE_SA_LEN
	msg.msg_namelen = dst->addr.sa.sa_len;
#else
	msg.msg_namelen = sizeof(dst->addr);
#endif
	msg.msg_iov     = &iov;
	msg.msg_iovlen  = 1;
	msg.msg_control = cmsgbuf;
	msg.msg_controllen = sizeof(cmsgbuf);
	msg.msg_flags = flags;

	cmsg = (struct cmsghdr *)cmsgbuf;
	switch (dst->addr.sa.sa_family) {
	case AF_INET:
		{
			struct in_pktinfo *pktinfo;

			cmsg->cmsg_level = IPPROTO_IP;
			cmsg->cmsg_type  = IP_PKTINFO;
			cmsg->cmsg_len   = CMSG_LEN(sizeof(struct in_pktinfo));
			pktinfo = (struct in_pktinfo *)(CMSG_DATA(cmsg));
			pktinfo->ipi_spec_dst = src->addr.si.sin_addr;

			break;
		}
#ifdef IPV6_PKTINFO
	case AF_INET6:
		{
			int ifindex;
			struct in6_pktinfo *pktinfo;

			if (IN6_IS_ADDR_LINKLOCAL(&src->addr.s6.sin6_addr) ||
			    IN6_IS_ADDR_MULTICAST(&src->addr.s6.sin6_addr))
			{
				ifindex = src->addr.s6.sin6_scope_id;
			} else {
				ifindex = 0;
			}

			msg.msg_controllen = CMSG_SPACE(sizeof(struct in6_pktinfo));
			cmsg->cmsg_level = IPPROTO_IPV6;
			cmsg->cmsg_type  = IPV6_PKTINFO;
			cmsg->cmsg_len   = CMSG_LEN(sizeof(struct in6_pktinfo));
			pktinfo = (struct in6_pktinfo *)(CMSG_DATA(cmsg));
			pktinfo->ipi6_addr = src->addr.s6.sin6_addr;
			pktinfo->ipi6_ifindex = ifindex;

			break;
		}
	}
#endif /* IPV6_PKTINFO */
	return sendmsg(sockfd, &msg, flags);
#else /* IP_PKTINFO */
	memset(src, 0, sizeof(*src));
	return sendto(sockfd, buffer, length, flags, &dst->addr.sa, dst->addrlen);
#endif /* IP_PKTINFO */
}

int
socket_close(int sockfd)
{
	return close(sockfd);
}
