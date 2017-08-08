#ifndef _COMMONNET_SSL_SETUP_H_
#define _COMMONNET_SSL_SETUP_H_

#include <openssl/ssl.h>

bool Ssl_thread_setup();
void Ssl_thread_clean();

extern SSL_CTX* global_ssl_ctx;

#endif //_COMMONNET_SSL_SETUP_H_
