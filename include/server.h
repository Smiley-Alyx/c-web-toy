#ifndef SERVER_H
#define SERVER_H

void start_server(int port);

#ifdef ENABLE_TLS
void start_server_tls(int port, const char* cert_file, const char* key_file);
#endif

#endif
