/* File: include/cyonnet.h
   Networking (sockets) helper prototypes.
*/

#ifndef CYONNET_H
#define CYONNET_H

#ifdef __cplusplus
extern "C" {
#endif

#include "cyonlib.h"

/* TCP helpers */
CYON_API int cyon_tcp_listen(const char *host, const char *port, int backlog);
CYON_API int cyon_tcp_accept(int listen_fd, char *peer_ip, size_t peer_ip_len, int *peer_port);
CYON_API int cyon_tcp_connect(const char *host, const char *port, int timeout_ms);

/* Send/receive helpers */
CYON_API int cyon_send_all(int fd, const void *buf, size_t len);
CYON_API ssize_t cyon_recv_all(int fd, void *buf, size_t len);

/* Socket flags */
CYON_API int cyon_socket_set_nonblocking(int fd, int nonblock);

#ifdef __cplusplus
}
#endif

#endif /* CYONNET_H */
