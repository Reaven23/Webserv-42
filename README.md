*This project has been created as part of the 42 curriculum by AlexisFaugeroux, Reaven23, SifexPro.*

# webserv

A non-blocking HTTP/1.1 server written in C++98, built from scratch. The server handles static file delivery, file uploads, directory listing, CGI execution, redirects, custom error pages, cookies, and sessions — all driven by a single `epoll()` event loop.

> **Platform support:** webserv is Unix-only. It relies on `epoll` (Linux) and on POSIX system calls (`fork`, `execve`, `pipe`, `socket`, ...). It does not build or run on Windows.

---

## Table of contents

- [Description](#description)
- [Features](#features)
- [Project structure](#project-structure)
- [Architecture](#architecture)
- [Request lifecycle](#request-lifecycle)
- [Why epoll](#why-epoll)
- [Instructions](#instructions)
- [Configuration file](#configuration-file)
- [Testing](#testing)

---

## Description

**webserv** is a 42 school project whose goal is to understand the HTTP protocol in depth by implementing a working HTTP server — the kind of software that sits behind every URL that starts with `http://`.

The server reads a configuration file (inspired by nginx syntax), binds one or more sockets, and handles HTTP requests using a single-process, single-threaded, event-driven architecture. It must:

- Never block.
- Never crash.
- Be compatible with real browsers.
- Support `GET`, `POST`, and `DELETE`.
- Execute CGI scripts (Python, Node.js).
- Be resilient under stress.

This implementation aims to follow the spirit of [RFC 9110 / 9112](https://www.rfc-editor.org/rfc/rfc9112.html) (HTTP semantics and message syntax), while staying within the subset required by the subject.

---

## Features

### Mandatory

- Single `epoll()` event loop driving all I/O (sockets, pipes).
- Multiple `server { }` blocks, each bound to its own `host:port`.
- Per-location routing: allowed methods, root, index, autoindex, redirects.
- Static file delivery with automatic MIME type detection.
- `GET`, `POST`, `DELETE` methods.
- File uploads via `multipart/form-data` (including max body size enforcement).
- CGI execution based on file extension, with environment variables per RFC 3875.
- HTTP redirects (301, 302, 307, 308).
- Custom and default error pages.
- Configurable `client_max_body_size` per server.
- Keep-alive, with idle connection timeout.
- Clean shutdown on `SIGINT` / `SIGTERM`.
- `SIGPIPE` ignored (so a disconnected client cannot kill the server).

### Bonus

- **Cookies & sessions** — in-memory session store with expiring session IDs.
- **Multiple CGI interpreters** — Python (`.py`) and Node.js (`.js`) side by side.

---

## Project structure

```
webserv/
|- includes/            # Header files (mirrors the srcs/ layout)
|  |- CGI/
|  |- config/
|  |- http/
|  |- network/
|  |- utils/
|  '- Webserv.hpp
|
|- srcs/
|  |- main.cpp             # Entry point
|  |- Webserv.cpp          # Top-level event loop
|  |
|  |- config/              # .conf parsing and validation
|  |- network/             # Sockets, accept(), epoll glue
|  |- http/                # Request parsing, method handlers, sessions
|  |- CGI/                 # Fork + execve + pipe wiring for CGI
|  '- utils/               # Logger, time helpers
|
|- config/                 # Sample configuration files
|  |- simple.conf
|  |- full.conf            # Feature-exhaustive config
|  |- cgi.conf
|  |- redirect.conf
|  '- ...
|
|- www/                    # Document roots, scripts, assets
|  |- html/                # Static pages, error pages
|  |- scripts/             # CGI scripts (.py, .js)
|  '- assets/
|
|- upload/                 # Default upload destination
|- Makefile
'- README.md
```

---

## Architecture

The server is organized around a handful of classes, each with a clear responsibility:

```
                       +-------------------+
                       |      Webserv      |    top-level object
                       |  - owns epoll fd  |    owns all Servers
                       |  - event loop     |
                       +---------+---------+
                                 |
                    +------------+------------+
                    |            |            |
              +-----v-----+ +----v------+ +---v-------+
              |  Server   | |  Server   | |  Server   |   one per listen block
              | :8080     | | :8081     | | :8084     |   owns a ServerSocket
              +-----+-----+ +-----------+ +-----------+   + its Clients
                    |
        +-----------+-----------+
        |           |           |
   +----v----+ +----v----+ +----v----+
   | Client  | | Client  | | Client  |   one per accepted TCP connection
   | fd 7    | | fd 9    | | fd 12   |   parses request, builds response
   +----+----+ +---------+ +---------+
        |
        |  (on CGI request)
   +----v----+
   |   CGI   |   one per in-flight CGI
   |  child  |   fork() + execve() + pipe()
   | process |
   +---------+
```

### Class responsibilities

| Class | Role |
|-------|------|
| `Webserv` | Owns the epoll instance and the list of `Server`s. Runs the event loop. |
| `Server` | One listening socket on one `host:port`. Accepts new clients, dispatches events. |
| `ServerSocket` | Thin RAII wrapper around `socket()` / `bind()` / `listen()`. |
| `Client` | Per-connection state: request buffer, parser, response, CGI registry. |
| `CGI` | Forks a child, pipes stdin/stdout, tracks its PID. |
| `Config` / `ServerConfig` / `LocationConfig` | Parsed configuration tree. |
| `IHttpHandler` and subclasses (`GetHttpHandler`, `PostHttpHandler`, `DeleteHttpHandler`) | Method-specific request handling. |
| `SessionManager` | In-memory session store with timeout-based cleanup. |
| `Logger` | Minimal leveled logger (info / warn / error). |

---

## Request lifecycle

The first three lines of the diagram below (`SYN`, `SYN-ACK`, `ACK`) are the TCP
three-way handshake — the conversation every TCP connection opens with, before
a single byte of HTTP is exchanged:

- **SYN** (*synchronize*) — "I want to open a connection. Here's my initial sequence number."
- **ACK** (*acknowledge*) — "I received your data up to this point."
- **SYN-ACK** — a single packet that combines the server's SYN with its ACK of the client's SYN.

The handshake is handled entirely by the kernel. Our code only sees the result:
`accept()` returns a new file descriptor once the handshake is complete.

```
  Client              Server (our process)                    CGI child
  ------              --------------------                    ---------

    |                         |
    |  SYN ------------------>|  (epoll reports listen fd)
    |                         |
    |<------------------- SYN-ACK
    |                         |
    |  ACK ------------------>|  accept() -> new Client fd
    |                         |  register fd with epoll (EPOLLIN)
    |                         |
    |  HTTP request --------->|  (epoll reports EPOLLIN on Client fd)
    |                         |  recv() -> append to buffer
    |                         |  parse: headers complete? body complete?
    |                         |
    |                         |  is it a CGI request?
    |                         |   |
    |                         |   +--- yes: fork()
    |                         |                      |
    |                         |                      +---> execve(script)
    |                         |  pipe <--------------+
    |                         |  read stdout via epoll until EOF
    |                         |  waitpid()
    |                         |
    |                         |  build HttpResponse
    |                         |  switch Client fd to EPOLLOUT
    |                         |
    |<--------- HTTP response |  send() chunks until buffer empty
    |                         |
    |                         |  keep-alive? reset state, back to EPOLLIN
    |                         |  otherwise close() the Client fd
```

### The event loop, in pseudocode

```cpp
while (!g_stop) {
    int n = epoll_wait(epollFd, events, MAX_EVENTS, timeout);

    for each server: server.closeIdleConnections();
    SessionManager::cleanup();

    for (int i = 0; i < n; ++i) {
        int fd = events[i].data.fd;

        if (fd is a listening socket) accept a new Client;
        else if (fd is a Client socket) {
            if (EPOLLIN)  read and parse request;
            if (EPOLLOUT) send response chunk;
        }
        else if (fd is a CGI pipe) {
            read CGI output, waitpid on EOF;
        }
    }
}
```

A single thread, a single `epoll_wait`, zero blocking reads or writes. File descriptors for sockets and pipes are set to non-blocking mode. Signal handlers set a `volatile sig_atomic_t` flag that the loop observes at each iteration.

---

## Why epoll

The subject allows `select`, `poll`, `epoll`, or `kqueue`. We picked `epoll` (Linux-specific) because it scales better than `select` / `poll` when the number of connections grows.

### The problem with `select` and `poll`

Both work on the same model: at every iteration, userspace hands the kernel **the full list of file descriptors to watch**, the kernel scans all of them, then returns the subset that are ready.

```
    userspace                           kernel
    ---------                           ------
    fds = [3, 5, 7, 9, 11, 12, 14]
                 |
                 v
    select(fds) --------------------->  scan fd 3  ... ready?
                                        scan fd 5  ... ready?
                                        scan fd 7  ... READY
                                        scan fd 9  ... ready?
                                        ...
    [ready fds] <-------------------
    userspace scans again to find which ones
```

Two costs per call: **O(n)** work in the kernel and **O(n)** work in userspace, every single time. `select` additionally has a hard cap of 1024 fds on most systems (`FD_SETSIZE`).

### The epoll model

`epoll` inverts the contract. Userspace registers fds with the kernel **once**, using `epoll_ctl(EPOLL_CTL_ADD)`. The kernel maintains a persistent "interest list" and an internal "ready list" updated as events happen. When userspace calls `epoll_wait`, the kernel just hands back the ready list — no scanning.

```
    userspace                           kernel
    ---------                           ------

    (once, at startup)
    epoll_ctl(ADD, fd3) ------------>  interest list: {3}
    epoll_ctl(ADD, fd5) ------------>  interest list: {3, 5}
    epoll_ctl(ADD, fd7) ------------>  interest list: {3, 5, 7}
                                       ...

    (every iteration)
    epoll_wait() -------------------->  ready list: [7, 12]
    <-- returns exactly 2 events
    userspace does O(2) work
```

- **Registration**: paid once per fd, not per iteration.
- **Wake-up cost**: O(number of ready events), not O(total watched fds).
- **No limit**: handles tens of thousands of fds without special tuning.

### Quick comparison

| Aspect | `select` | `poll` | `epoll` |
|--------|----------|--------|---------|
| Per-call cost | O(n) | O(n) | O(ready) |
| Max fds | ~1024 (`FD_SETSIZE`) | unlimited | unlimited |
| Fd list passed every call | yes | yes | no (persistent in kernel) |
| Portable | yes (POSIX) | yes (POSIX) | Linux only |
| Edge / level triggered | level only | level only | both |

### Edge-triggered vs level-triggered

`epoll` can be used in two modes:

- **Level-triggered (default):** `epoll_wait` reports an fd as long as it has pending data/space. Easier to use — read/write what you can, you'll be notified again next iteration if there's more.
- **Edge-triggered (`EPOLLET`):** you're notified only once per transition from "not ready" to "ready". You **must** drain the fd completely in one go, or you'll hang waiting for a notification that never comes. Faster but harder to get right.

webserv uses **level-triggered** mode: it's simpler, reliable, and the performance difference is irrelevant at the scale of a 42 evaluation.

---

## Instructions

### Build

```bash
make          # build ./webserv
make clean    # remove object files
make fclean   # remove objects and binary
make re       # fclean + make
```

The binary is compiled with `-Wall -Wextra -Werror -std=c++98`.

### Run

```bash
./webserv [configuration file]
```

Example:

```bash
./webserv config/full.conf
```

Then open your browser at `http://localhost:8080/`.

### Shutdown

Press `Ctrl+C`. The server catches `SIGINT`, drains its destructors, closes all sockets, and exits with status 0.

---

## Configuration file

The configuration syntax is inspired by nginx. A minimal example:

```nginx
server {
    listen 8080;
    server_name myserver;
    root ./www;
    index index.html;

    client_max_body_size 2000000;

    error_page 404 ./www/html/errors/404.html;

    location / {
        methods GET POST DELETE;
    }

    location /upload {
        methods POST;
        upload on;
    }

    location /cgi {
        methods GET POST;
        cgi .py;
    }

    location /old-page {
        redirect 301 /;
    }
}
```

### Supported directives

**Server-level:**

| Directive | Description |
|-----------|-------------|
| `listen <port>;` | Bind to the given port. |
| `server_name <name>;` | Server name (informational). |
| `root <path>;` | Default document root. |
| `index <file>;` | Default file served when a directory is requested. |
| `client_max_body_size <bytes>;` | Maximum accepted request body size. |
| `error_page <code> <path>;` | Custom error page for the given status code. |

**Location-level:**

| Directive | Description |
|-----------|-------------|
| `methods <GET\|POST\|DELETE>;` | Allowed HTTP methods. |
| `root <path>;` | Per-location document root (overrides server `root`). |
| `index <file>;` | Per-location index file. |
| `autoindex <on\|off>;` | Enable directory listing. |
| `redirect <code> <target>;` | Return an HTTP redirect. |
| `cgi <extension>;` | Treat files with this extension as CGI. |
| `upload <on\|off>;` | Allow uploads to this location. |

---

## Testing

The `config/` directory contains ready-to-use configurations covering each feature in isolation (CGI, uploads, redirects, error pages, multiple ports, ...). `config/full.conf` exposes every feature at once and is the reference for manual testing.

Browser-based dashboard:

```bash
./webserv config/full.conf
# Open http://localhost:8080/
```

Example commands:

```bash
# Static file
curl -i http://localhost:8080/

# Upload
curl -i -X POST -F "file=@photo.jpg" http://localhost:8080/upload

# Delete
curl -i -X DELETE http://localhost:8080/files/photo.jpg

# CGI
curl -i http://localhost:8084/py-cgi/echo.py?hello=world
curl -i http://localhost:8084/js-cgi/clearUpload.js

# Redirects
curl -i http://localhost:8081/old-page
curl -i http://localhost:8081/temp-page

# Body limit (should return 413)
curl -i -X POST --data-binary @<(head -c 3000000 /dev/zero) \
     http://localhost:8080/upload
```
