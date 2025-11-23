#include "cyonstd.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>

/* Create a TCP listening socket on host:port.
   host may be NULL for INADDR_ANY. Returns socket fd >=0 or negative errno-like. */
int cyon_tcp_listen(const char *host, const char *port, int backlog) {
    struct addrinfo hints;
    struct addrinfo *res = NULL, *rp;
    int sfd = -1;
    int rc;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    rc = getaddrinfo(host, port, &hints, &res);
    if (rc != 0) return EAI_SYSTEM;

    for (rp = res; rp != NULL; rp = rp->ai_next) {
        sfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (sfd == -1) continue;
        int opt = 1;
        setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
        if (bind(sfd, rp->ai_addr, rp->ai_addrlen) == 0) {
            if (listen(sfd, backlog) == 0) {
                freeaddrinfo(res);
                return sfd;
            }
        }
        close(sfd);
        sfd = -1;
    }

    freeaddrinfo(res);
    return errno ? errno : -1;
}

/* Accept connection on listening fd. Returns new fd or negative errno-like. */
int cyon_tcp_accept(int listen_fd, char *peer_ip, size_t peer_ip_len, int *peer_port) {
    if (listen_fd < 0) return EINVAL;
    struct sockaddr_storage addr;
    socklen_t addrlen = sizeof(addr);
    int cfd = accept(listen_fd, (struct sockaddr*)&addr, &addrlen);
    if (cfd < 0) return errno ? errno : -1;

    if (peer_ip) {
        void *inaddr = NULL;
        if (addr.ss_family == AF_INET) {
            struct sockaddr_in *a = (struct sockaddr_in*)&addr;
            inaddr = &a->sin_addr;
            if (peer_port) *peer_port = ntohs(a->sin_port);
        } else {
            struct sockaddr_in6 *a = (struct sockaddr_in6*)&addr;
            inaddr = &a->sin6_addr;
            if (peer_port) *peer_port = ntohs(a->sin6_port);
        }
        inet_ntop(addr.ss_family, inaddr, peer_ip, peer_ip_len);
    }

    return cfd;
}

/* Connect to remote host:port. Returns connected socket fd or negative errno-like. */
int cyon_tcp_connect(const char *host, const char *port, int timeout_ms) {
    struct addrinfo hints;
    struct addrinfo *res = NULL, *rp;
    int sfd = -1;
    int rc;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    rc = getaddrinfo(host, port, &hints, &res);
    if (rc != 0) return EAI_SYSTEM;

    for (rp = res; rp != NULL; rp = rp->ai_next) {
        sfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (sfd == -1) continue;

        /* optionally set non-blocking and implement timeout */
        if (timeout_ms > 0) {
            int flags = fcntl(sfd, F_GETFL, 0);
            if (flags >= 0) fcntl(sfd, F_SETFL, flags | O_NONBLOCK);
        }

        int r = connect(sfd, rp->ai_addr, rp->ai_addrlen);
        if (r == 0) {
            /* connected immediately */
            if (timeout_ms > 0) {
                int flags = fcntl(sfd, F_GETFL, 0);
                if (flags >= 0) fcntl(sfd, F_SETFL, flags & ~O_NONBLOCK);
            }
            freeaddrinfo(res);
            return sfd;
        } else {
            if (errno == EINPROGRESS && timeout_ms > 0) {
                /* wait with select */
                fd_set wf;
                FD_ZERO(&wf);
                FD_SET(sfd, &wf);
                struct timeval tv;
                tv.tv_sec = timeout_ms / 1000;
                tv.tv_usec = (timeout_ms % 1000) * 1000;
                int sel = select(sfd + 1, NULL, &wf, NULL, &tv);
                if (sel > 0 && FD_ISSET(sfd, &wf)) {
                    int err = 0;
                    socklen_t len = sizeof(err);
                    if (getsockopt(sfd, SOL_SOCKET, SO_ERROR, &err, &len) == 0 && err == 0) {
                        int flags = fcntl(sfd, F_GETFL, 0);
                        if (flags >= 0) fcntl(sfd, F_SETFL, flags & ~O_NONBLOCK);
                        freeaddrinfo(res);
                        return sfd;
                    }
                }
            }
        }
        close(sfd);
        sfd = -1;
    }

    freeaddrinfo(res);
    return errno ? errno : -1;
}

/* Send all bytes from buffer, loop until complete or error.
   Returns 0 on success, or errno-like on failure. */
int cyon_send_all(int fd, const void *buf, size_t len) {
    if (fd < 0 || (!buf && len > 0)) return EINVAL;
    const unsigned char *p = (const unsigned char*)buf;
    size_t sent = 0;
    while (sent < len) {
        ssize_t r = send(fd, p + sent, len - sent, 0);
        if (r < 0) {
            if (errno == EINTR) continue;
            return errno ? errno : -1;
        }
        if (r == 0) return EPIPE;
        sent += (size_t)r;
    }
    return 0;
}

/* Receive exact len bytes unless connection closes. Caller buffer must be large enough.
   Returns number of bytes read on success (==len) or negative errno-like on error. */
ssize_t cyon_recv_all(int fd, void *buf, size_t len) {
    if (fd < 0 || (!buf && len > 0)) return -1;
    unsigned char *p = (unsigned char*)buf;
    size_t got = 0;
    while (got < len) {
        ssize_t r = recv(fd, p + got, len - got, 0);
        if (r < 0) {
            if (errno == EINTR) continue;
            return -(errno ? errno : -1);
        }
        if (r == 0) break;
        got += (size_t)r;
    }
    return (ssize_t)got;
}

/* Set socket to non-blocking mode. Returns 0 on success. */
int cyon_socket_set_nonblocking(int fd, int nonblock) {
    if (fd < 0) return EINVAL;
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0) return errno ? errno : -1;
    if (nonblock) flags |= O_NONBLOCK;
    else flags &= ~O_NONBLOCK;
    if (fcntl(fd, F_SETFL, flags) != 0) return errno ? errno : -1;
    return 0;
}