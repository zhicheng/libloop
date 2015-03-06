#include "buf.h"

#include <errno.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>

/* can't bigger than UIO_MAXIOV */
#define IOVCNT_DEFAULT 16

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
                len = 1;        /* don't error when send block */
	}

        return len;
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

        return len;
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

long
cbuf_get_off(cbuf_t *cbuf)
{
	int i;
	long off;

	off = 0;
	for (i = 0; i < cbuf->off; i++) {
		off += cbuf->buf[i].len;
	}

	off += cbuf->buf[cbuf->off].off;
	
	return off;
}

void
cbuf_set_off(cbuf_t *cbuf, long off)
{
	int i;

	for (i = 0; i < cbuf->len; i++) {
		if (cbuf->buf[i].len > off) {
			cbuf->buf[i].off = off;

			break;
		} else {
			off -= cbuf[i].len;
		}
	}
}

long
cbuf_get_len(cbuf_t *cbuf)
{
	int i;
	long len;

	len = 0;
	for (i = 0; i < cbuf->len; i++) {
		len += cbuf->buf[i].len;
	}

	return len;
}

void
cbuf_set_len(cbuf_t *cbuf, long off)
{
	int i;
	long len;

	len = 0;
	for (i = 0; i < cbuf->max; i++) {
		if (cbuf->buf[i].max > len) {
			cbuf->buf[i].len = len;

			break;
		} else {
			len -= cbuf->buf[i].max;
		}
	}
}

long
cbuf_get_max(cbuf_t *cbuf)
{
	int i;
	long max;

	max = 0;
	for (i = 0; i < cbuf->max; i++) {
		max += cbuf->buf[i].max;
	}

	return max;
}

int
cbuf_extend(cbuf_t *cbuf, int len)
{
	sbuf_t *buf;

	if ((cbuf->len + len) < cbuf->max) {
		return 0;
	}

	if (cbuf->buf != NULL) {
		buf = realloc(cbuf->buf, (cbuf->max + len) * sizeof(sbuf_t));
	} else {
		buf = malloc(len * sizeof(sbuf_t));
	}
	if (buf != NULL) {
		cbuf->buf  = buf;
		cbuf->max += len;
	} else {
		return -1;
	}

	return 0;
}

void
cbuf_iovec(cbuf_t *cbuf, struct iovec *iov, int iovcnt, int type)
{
	int i;
	int off;
	int len;

	switch (type) {
	case 0:
		off = 0;
		len = cbuf->len;

		break;
	case 1:
		off = cbuf->off;
		len = cbuf->len;

		break;
	case 2:
		off = cbuf->len;
		len = cbuf->max;

		break;
	}

	for (i = 0; i < iovcnt && off < len; i++, off++) {
		iov[i].iov_len  = cbuf->buf[off].len;
		iov[i].iov_base = cbuf->buf[off].buf + cbuf->buf[off].off;
	}
}

ssize_t
cbuf_send(const int fd, cbuf_t *buf, ssize_t *snd)
{
	long off;
        ssize_t n;

	int iovcnt;
	struct iovec iov[IOVCNT_DEFAULT];


        assert(buf->len <= buf->max);
        assert(buf->off <  buf->len);

        *snd = 0;
	iovcnt = IOVCNT_DEFAULT;
	off = cbuf_get_off(buf);
        do {
		if ((buf->len - buf->off) < iovcnt) {
			iovcnt = buf->len - buf->off;
		}
		cbuf_iovec(buf, iov, iovcnt, 1);

		n = writev(fd, iov, iovcnt);
                if (n > 0) {
                        *snd += n;
                         off += n;
                }
        } while (off > 0 && off < cbuf_get_len(buf) && n > 0);

	cbuf_set_off(buf, off);

        if (n == -1 && errno == EWOULDBLOCK) {
                n = 1;        /* don't error when writev block */
	}

        return n;
}

ssize_t
cbuf_recv(const int fd, cbuf_t *buf, ssize_t *rcv)
{
	long len;
        ssize_t n;

	int iovcnt;
	struct iovec iov[IOVCNT_DEFAULT];

        assert(buf->len < buf->max);

        *rcv = 0;
	iovcnt = IOVCNT_DEFAULT;
	len = cbuf_get_len(buf);
        do {
		if ((buf->len - buf->off) < iovcnt) {
			iovcnt = buf->len - buf->off;
		}
		cbuf_iovec(buf, iov, iovcnt, 2);

		n = readv(fd, iov, iovcnt);
                if (n > 0) {
                        *rcv += n;
                         len += n;
                }
        } while (n > 0 && len < cbuf_get_max(buf) && n > 0);

	cbuf_set_len(buf, len);

        if (n == -1 && errno == EWOULDBLOCK) {
                n = 1;        /* don't error when readv block */
	}

        return n;
}
