#include "ssl.h"

#include <openssl/bio.h> 
#include <openssl/ssl.h> 
#include <openssl/err.h> 

struct ssl_ctx {
	SSL_CTX *ctx;
};

struct ssl {
	SSL *ssl;
};

struct ssl_ctx *
ssl_ctx_init(char *certificate, char *private)
{
	struct ssl_ctx *ctx;

	ctx = malloc(sizeof(struct ssl_ctx));

	SSL_load_error_strings();
	SSL_library_init();
	OpenSSL_add_all_algorithms();

	ctx->ctx = SSL_CTX_new(SSLv23_server_method());
	SSL_CTX_set_options(ctx->ctx, SSL_OP_SINGLE_DH_USE);
	SSL_CTX_use_certificate_file(ctx->ctx, certificate, SSL_FILETYPE_PEM);
	SSL_CTX_use_PrivateKey_file(ctx->ctx,  private,     SSL_FILETYPE_PEM);
/*
	SSL_CTX_set_cipher_list(ctx->ctx, "ALL");
*/

	return ctx;
}

void
ssl_ctx_deinit(struct ssl_ctx *ctx)
{
	ERR_free_strings();
	EVP_cleanup();

	free(ctx);
}

struct ssl *
ssl_open(struct ssl_ctx *ctx, int fd)
{
	struct ssl *ssl;
	ssl = malloc(sizeof(struct ssl));

	ssl->ssl = SSL_new(ctx->ctx);
	SSL_set_fd(ssl->ssl, fd);

	return ssl;
}

int
ssl_accept(struct ssl *ssl)
{
	return SSL_accept(ssl->ssl);
}

void
ssl_close(struct ssl *ssl)
{
	SSL_shutdown(ssl->ssl);
	SSL_free(ssl->ssl);
	free(ssl);
}

ssize_t
ssl_read(struct ssl *ssl, void *buf, size_t n)
{
	return SSL_read(ssl->ssl, buf, n);
}

ssize_t
ssl_write(struct ssl *ssl, void *buf, size_t n)
{
	return SSL_write(ssl->ssl, buf, n);
}

int
ssl_error(struct ssl *ssl, int ret)
{
	return SSL_get_error(ssl->ssl, ret);
}
