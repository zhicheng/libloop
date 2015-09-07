#ifndef __BUF_H__
#define __BUF_H__

#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/uio.h>

/* TODO: add a sbuf_ctx_t and cbuf_ctx_t for allocator */
/* FIXME: cbuf is not well tested do not use */

/* should reset sbuf when off == len to save memory */
typedef struct sbuf {    /* stream buffer     */
	int off;         /* increase by write */
	int len;         /* increase by read  */
	int max;         /* off < len <= max  */
	char *buf;
} sbuf_t;

typedef struct cbuf {    /* chained buffer   */
	int off;         /* not byte         */
	int len;         /* not byte         */
	int max;         /* off < len <= max */
	sbuf_t *buf;
} cbuf_t;

int
sbuf_alloc(sbuf_t *sbuf, int max);

void
sbuf_release(sbuf_t *sbuf);

/*
 * sbuf->off = 0;
 * sbuf->len = 0;
 * keep sbuf->max and sbuf->buf
 */
void
sbuf_reset(sbuf_t *sbuf);

char *
sbuf_detach(sbuf_t *sbuf);

/*
 * free sbuf->buf
 * sbuf->buf = buf;
 * sbuf->max = len;
 */
void
sbuf_attach(sbuf_t *sbuf, char *buf, int len);

/*
 * make buf->max >= buf->len + len
 */
int
sbuf_extend(sbuf_t *sbuf, int len);

int
sbuf_prepend(sbuf_t *sbuf, void *buf, int len);

int
sbuf_append(sbuf_t *sbuf, void *buf, int len);

/*
 * send until empty the sbuf or will block or full
 * return  > 0 success
 * return <= 0 fail
 */
ssize_t
sbuf_send(const int fd, sbuf_t *buf, ssize_t *snd);

/*
 * recv until full the sbuf or will block
 * return  > 0 success
 * return <= 0 fail
 */
ssize_t
sbuf_recv(const int fd, sbuf_t *buf, ssize_t *rcv);

/* TODO: add sbuf interface to cbuf */

int
cbuf_alloc(cbuf_t *cbuf, int max);

void
cbuf_release(cbuf_t *cbuf);

/* bytes offset */
long
cbuf_get_off(cbuf_t *cbuf);

void
cbuf_set_off(cbuf_t *cbuf, long off);

/* bytes length */
long
cbuf_get_len(cbuf_t *cbuf);

void
cbuf_set_len(cbuf_t *cbuf, long off);

/*
 * bytes max
 * and there is no set_max
 */
long
cbuf_get_max(cbuf_t *cbuf);

int
cbuf_extend(cbuf_t *cbuf, int len);

/*
 * If type is 0, the offset is from start (0 -> len).
 * If type is 1, the offset is from current location (off -> len).
 * If type is 2, the offset is from end location (len -> max).
 * If type is none above, the offset is from start (0 -> len).
 */
void
cbuf_iovec(cbuf_t *cbuf, struct iovec *iov, int iovcnt, int type);

/*
 * send until empty the cbuf or will block or full
 * return  > 0 success
 * return <= 0 fail
 */
ssize_t
cbuf_send(const int fd, cbuf_t *buf, ssize_t *snd);

/*
 * recv until full the cbuf or will block
 * return  > 0 success
 * return <= 0 fail
 */
ssize_t
cbuf_recv(const int fd, cbuf_t *buf, ssize_t *rcv);

#endif /* __BUF_H__ */
