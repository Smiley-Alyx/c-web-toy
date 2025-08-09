#include "session.h"
#include "http.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>

#define MAX_SESSIONS 256

static Session g_sessions[MAX_SESSIONS];
static int g_inited = 0;

static void random_hex32(char out[33]) {
    unsigned char buf[16];
    int fd = open("/dev/urandom", O_RDONLY);
    if (fd >= 0) {
        ssize_t r = read(fd, buf, sizeof(buf));
        close(fd);
        if (r == 16) {
            static const char* hex = "0123456789abcdef";
            for (int i = 0; i < 16; i++) {
                out[i*2]   = hex[(buf[i] >> 4) & 0xF];
                out[i*2+1] = hex[buf[i] & 0xF];
            }
            out[32] = '\0';
            return;
        }
    }
    // Fallback to rand()
    srand((unsigned)time(NULL) ^ (unsigned)getpid());
    static const char* hex = "0123456789abcdef";
    for (int i = 0; i < 32; i++) out[i] = hex[rand() & 0xF];
    out[32] = '\0';
}

void sessions_init(void) {
    if (g_inited) return;
    memset(g_sessions, 0, sizeof(g_sessions));
    g_inited = 1;
}

static Session* find_by_id(const char* sid) {
    for (int i = 0; i < MAX_SESSIONS; i++) {
        if (g_sessions[i].in_use && strncmp(g_sessions[i].id, sid, 32) == 0)
            return &g_sessions[i];
    }
    return NULL;
}

static Session* create_session(void) {
    for (int i = 0; i < MAX_SESSIONS; i++) {
        if (!g_sessions[i].in_use) {
            g_sessions[i].in_use = 1;
            random_hex32(g_sessions[i].id);
            g_sessions[i].counter = 0;
            return &g_sessions[i];
        }
    }
    return NULL; // out of capacity (ok for demo)
}

Session* session_get_or_create(HttpRequest* req, HttpResponse* res) {
    sessions_init();

    const char* sid = http_get_cookie(req, "sid");
    if (sid) {
        Session* s = find_by_id(sid);
        if (s) return s;
        // fallthrough: invalid/expired sid → create new
    }
    Session* s = create_session();
    if (!s) return NULL;

    // Set cookie; secure attributes kept minimal for demo
    http_set_cookie(res, "sid", s->id, "Path=/; HttpOnly; SameSite=Lax; Max-Age=86400");
    return s;
}
