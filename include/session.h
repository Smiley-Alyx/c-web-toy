#ifndef SESSION_H
#define SESSION_H

#include "http.h"

typedef struct {
    char id[33];
    int counter;
    int in_use;
} Session;

void sessions_init(void);
Session* session_get_or_create(HttpRequest* req, HttpResponse* res);

#endif
