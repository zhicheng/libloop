#ifndef __SSL_H__
#define __SSL_H__

#include <stdio.h>
#include <stddef.h>
#include <stdint.h>

struct ssl_ctx *
ssl_ctx_init(char *certificate, char *private);

void
ssl_ctx_deinit(struct ssl_ctx *ctx);

struct ssl *
ssl_open(struct ssl_ctx *ctx, int fd);

int
ssl_accept(struct ssl *ssl);

void
ssl_close(struct ssl *ssl);

ssize_t
ssl_read(struct ssl *ssl, void *buf, size_t n);

ssize_t
ssl_write(struct ssl *ssl, void *buf, size_t n);

int
ssl_error();

#endif /* __SSL_H__ */
