#include "tls.h"
#include <openssl/err.h>

int tls_init(TlsContext* t, const char* cert_file, const char* key_file) {
    SSL_library_init();
    SSL_load_error_strings();
    OpenSSL_add_all_algorithms();

    const SSL_METHOD* method = TLS_server_method();
    t->ctx = SSL_CTX_new(method);
    if (!t->ctx) return -1;

    if (SSL_CTX_use_certificate_file(t->ctx, cert_file, SSL_FILETYPE_PEM) <= 0) return -2;
    if (SSL_CTX_use_PrivateKey_file(t->ctx, key_file, SSL_FILETYPE_PEM) <= 0)   return -3;
    if (!SSL_CTX_check_private_key(t->ctx))                                      return -4;

    // Reasonable defaults
    SSL_CTX_set_min_proto_version(t->ctx, TLS1_2_VERSION);
    return 0;
}

SSL* tls_accept_client(TlsContext* t, int client_fd) {
    SSL* ssl = SSL_new(t->ctx);
    if (!ssl) return NULL;
    SSL_set_fd(ssl, client_fd);
    if (SSL_accept(ssl) <= 0) {
        SSL_free(ssl);
        return NULL;
    }
    return ssl;
}

void tls_free(TlsContext* t) {
    if (t->ctx) SSL_CTX_free(t->ctx);
    t->ctx = NULL;
    EVP_cleanup();
    ERR_free_strings();
}
