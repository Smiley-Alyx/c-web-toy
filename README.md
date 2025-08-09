# c-web-toy

[🇷🇺 Russian version](README.ru.md) | [🇬🇧 English version](README.md)

A **learning micro web framework in pure C**. The goal is to understand how the web works: sockets, HTTP parsing, routing, static files, a minimal template engine.

> Code comments and messages — **in English**. Documentation — **in both Russian and English**.

## Structure

```
include/   — header files (public API)
src/       — source code
public/    — static files (HTML/CSS/JS)
templates/ — templates (for later stages)
```

## Build and run

```bash
make
./server
```

By default, the server listens on `http://localhost:8080`.

## HTTPS support

To run with HTTPS in parallel with HTTP:

```bash
# Generate self-signed certificate
openssl req -x509 -nodes -newkey rsa:2048 -keyout key.pem -out cert.pem -days 365

# Run with HTTP (8080) + HTTPS (8443)
PORT=8080 HTTPS=1 HTTPS_PORT=8443 CERT_FILE=./cert.pem KEY_FILE=./key.pem ./server
```

## Features (current stage)

- Simple routing by method + path (`GET /hello`)
- Static file serving with proper `Content-Type`: `mount /static/ -> ./public`
- Minimal `{{var}}` template engine for HTML strings (no file loading)
- Minimal HTTP parsing (method, path, headers, query parameters)
- File-based template rendering (`render_template_file`)
- POST/forms (`application/x-www-form-urlencoded`)
- Cookies/sessions
- Parallel HTTP + HTTPS listeners

## Roadmap

- [x] Basic server, request parsing
- [x] Routing `method + path`
- [x] Static files (`/static/ -> ./public`)
- [x] Template engine (strings)
- [x] Template engine from files (`render_template_file`)
- [x] POST/forms (`application/x-www-form-urlencoded`)
- [x] Cookies/sessions
- [x] HTTPS support
- [ ] Demo application

## License

MIT
