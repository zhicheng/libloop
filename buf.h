#ifndef __BUF_H__
#define __BUF_H__

#include <stdio.h>
#include <stddef.h>
#include <stdint.h>

/* TODO: add a sbuf_ctx_t and cbuf_ctx_t for allocator */

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
 * send until empty the sbuf or will block
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
cbuf_alloc(cbuf_t *cbuf, int max, int sbuf_max);

void
cbuf_release(cbuf_t *cbuf);

#endif /* __BUF_H__ */
