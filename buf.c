#include "buf.h"

#include <errno.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>

int
sbuf_alloc(sbuf_t *sbuf, int max)
{
	sbuf->off = 0;
	sbuf->len = 0;

	if ((sbuf->max = max) > 0 &&
	    (sbuf->buf = malloc(max)) != NULL)
	{
		return 0;
	}
	sbuf->max = 0;

	return -1;
}

void
sbuf_release(sbuf_t *sbuf)
{
	sbuf->off = 0;
	sbuf->len = 0;
	sbuf->max = 0;
	free(sbuf->buf);
	sbuf->buf = NULL;
}

void
sbuf_reset(sbuf_t *sbuf)
{
	sbuf->off = 0;
	sbuf->len = 0;
}

char *
sbuf_detach(sbuf_t *sbuf)
{
	char *buf;

	buf = sbuf->buf;

	sbuf->off = 0;
	sbuf->len = 0;
	sbuf->max = 0;
	sbuf->buf = NULL;

	return buf;
}

void
sbuf_attach(sbuf_t *sbuf, char *buf, int len)
{
	sbuf_release(sbuf);

	sbuf->buf = buf;
	sbuf->len = len;
	sbuf->max = len;
}

int
sbuf_extend(sbuf_t *sbuf, int len)
{
	char *buf;

	if ((sbuf->len + len) < sbuf->max) {
		return 0;
	}

	if (sbuf->buf != NULL) {
		buf = realloc(sbuf->buf, sbuf->max + len);
	} else {
		buf = malloc(len);
	}
	if (buf != NULL) {
		sbuf->buf  = buf;
		sbuf->max += len;
	} else {
		return -1;
	}

	return 0;
}

int
sbuf_prepend(sbuf_t *sbuf, void *buf, int len)
{
	sbuf_extend(sbuf, len);

	memmove(sbuf->buf + len, sbuf->buf, len);
	memcpy(sbuf->buf + sbuf->off, buf, len);

	return 0;
}

int
sbuf_append(sbuf_t *sbuf, void *buf, int len)
{
	sbuf_extend(sbuf, len);

	memcpy(sbuf->buf + sbuf->len, buf, len);
	sbuf->len += len;

	return 0;
}

ssize_t
sbuf_send(const int fd, sbuf_t *buf, ssize_t *snd)
{
        ssize_t len;

        assert(buf->len <= buf->max);
        assert(buf->off <  buf->len);

        *snd = 0;
        do {
                len = send(fd, buf->buf + buf->off, buf->len - buf->off, 0);
                if (len > 0) {
                        *snd     += len;
                        buf->off += len;
                }
        } while (buf->off > 0 && buf->off < buf->len && len > 0);

        if (len == -1 && errno == EWOULDBLOCK) {
                len = 1;        /* don't error when recv block */
	}

        return len;     /* err */
}

ssize_t
sbuf_recv(const int fd, sbuf_t *buf, ssize_t *rcv)
{
        ssize_t len;

        assert(buf->len < buf->max);

        *rcv = 0;
        do {
                len = recv(fd, buf->buf + buf->len, buf->max - buf->len, 0);
                if (len > 0) {
                        *rcv     += len;
                        buf->len += len;
                }
        } while (buf->len > 0 && buf->len < buf->max && len > 0);

        if (len == -1 && errno == EWOULDBLOCK) {
                len = 1;        /* don't error when recv block */
	}

        return len;     /* err */
}

int
cbuf_alloc(cbuf_t *cbuf, int max)
{
	cbuf->off = 0;
	cbuf->len = 0;

	if ((cbuf->max = max) > 0 &&
	    (cbuf->buf = malloc(max * sizeof(sbuf_t))) != NULL)
	{
		return 0;
	}
	cbuf->max = 0;

	return -1;
}

void
cbuf_release(cbuf_t *cbuf)
{
	cbuf->off = 0;
	cbuf->len = 0;
	cbuf->max = 0;
	free(cbuf->buf);
	cbuf->buf = NULL;
}

void
cbuf_iovec(cbuf_t *cbuf, struct iovec *iov, int iovcnt)
{
	int i;

	for (i = 0; i < cbuf->len; i++) {
		iov[i].iov_len  = cbuf->buf[i].len;
		iov[i].iov_base = cbuf->buf[i].buf + cbuf->buf[i].off;
	}
}
