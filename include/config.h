#ifndef CONFIG_H
#define CONFIG_H

#include <stdbool.h>

int  config_get_port(int default_port);                          // PORT
void config_get_static(char* url_prefix, int up_len,             // STATIC_URL_PREFIX
                       char* dir, int dir_len);                  // STATIC_DIR
void config_dump(void);                                          // logs current config

#endif
