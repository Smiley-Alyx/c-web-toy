#include "static.h"
#include "mime.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

// Very simple single-mount implementation for now
static char g_url_prefix[128] = "/static/";
static char g_base_dir[512]   = "./public";

static int path_starts_with(const char* path, const char* prefix) {
    size_t lp = strlen(prefix);
    return strncmp(path, prefix, lp) == 0;
}

// Reject any path containing ".." segments to prevent traversal
static int is_safe_subpath(const char* p) {
    return strstr(p, "..") == NULL;
}

void static_mount(const char* url_prefix, const char* dir) {
    strncpy(g_url_prefix, url_prefix, sizeof(g_url_prefix)-1);
    g_url_prefix[sizeof(g_url_prefix)-1] = '\0';
    strncpy(g_base_dir, dir, sizeof(g_base_dir)-1);
    g_base_dir[sizeof(g_base_dir)-1] = '\0';
}

int static_try_serve(HttpRequest* req, HttpResponse* res) {
    if (!path_starts_with(req->path, g_url_prefix)) return 0;

    const char* rel = req->path + strlen(g_url_prefix);
    if (!is_safe_subpath(rel)) {
        const char* msg = "403 Forbidden";
        char buf[256];
        snprintf(buf, sizeof(buf),
            "HTTP/1.1 403 Forbidden\r\nContent-Length: %lu\r\nConnection: close\r\n\r\n%s",
            (unsigned long)strlen(msg), msg);
        write(res->client_fd, buf, strlen(buf));
        return 1;
    }

    // Build filesystem path
    char fs_path[1024];
    if (rel[0] == '\0') rel = "index.html";           // /static/ -> index.html
    snprintf(fs_path, sizeof(fs_path), "%s/%s", g_base_dir, rel);

    // If path ends with '/', serve index.html
    size_t len = strlen(fs_path);
    if (len > 0 && fs_path[len-1] == '/') {
        strncat(fs_path, "index.html", sizeof(fs_path) - strlen(fs_path) - 1);
    }

    // Stat file
    struct stat st;
    if (stat(fs_path, &st) != 0 || !S_ISREG(st.st_mode)) {
        // Not found
        const char* msg = "404 Not Found";
        char buf[256];
        snprintf(buf, sizeof(buf),
            "HTTP/1.1 404 Not Found\r\nContent-Length: %lu\r\nConnection: close\r\n\r\n%s",
            (unsigned long)strlen(msg), msg);
        write(res->client_fd, buf, strlen(buf));
        return 1;
    }

    // Open and read
    int fd = open(fs_path, O_RDONLY);
    if (fd < 0) {
        const char* msg = "500 Internal Server Error";
        char buf[256];
        snprintf(buf, sizeof(buf),
            "HTTP/1.1 500 Internal Server Error\r\nContent-Length: %lu\r\nConnection: close\r\n\r\n%s",
            (unsigned long)strlen(msg), msg);
        write(res->client_fd, buf, strlen(buf));
        return 1;
    }

    // Allocate buffer and read file
    size_t size = (size_t)st.st_size;
    unsigned char* data = (unsigned char*)malloc(size);
    if (!data) {
        close(fd);
        const char* msg = "500 Internal Server Error";
        char buf[256];
        snprintf(buf, sizeof(buf),
            "HTTP/1.1 500 Internal Server Error\r\nContent-Length: %lu\r\nConnection: close\r\n\r\n%s",
            (unsigned long)strlen(msg), msg);
        write(res->client_fd, buf, strlen(buf));
        return 1;
    }

    size_t read_total = 0;
    while (read_total < size) {
        ssize_t r = read(fd, data + read_total, size - read_total);
        if (r <= 0) break;
        read_total += (size_t)r;
    }
    close(fd);

    const char* mime = mime_from_path(fs_path);
    http_send_bytes(res, mime, data, read_total);
    free(data);
    return 1;
}
