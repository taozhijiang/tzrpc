#ifndef __UTILS_SSL_SETUP_H__
#define __UTILS_SSL_SETUP_H__

#include <openssl/ssl.h>

bool Ssl_thread_setup();
void Ssl_thread_clean();

extern SSL_CTX* global_ssl_ctx;

#endif //__UTILS_SSL_SETUP_H__
