/*
 * Copyright (C) Cihangir Akturk 2012
 *
 * This file is part of chngr's simple stupid web server (ssws).
 *
 * ssws is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * ssws is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with ssws.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h> /* former <arpa/inet.h>*/
#include <netdb.h>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "http_header_parser.h"
#include "server_mem.h"

static struct server_buf app_buf;

static inline size_t get_fsize(int fd)
{
    size_t len = (size_t)lseek(fd, 0, SEEK_END);
    lseek(fd, 0, SEEK_SET);
    return len;
}

static int get_server_fd(const char *port)
{
    struct addrinfo hints;
    struct addrinfo *result, *rp;
    int fd = -1, t = 1;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if (getaddrinfo(NULL, port, &hints, &result) != 0)
        return fd;

    for (rp = result; rp != NULL; rp = rp->ai_next) {
        fd = socket(rp->ai_family, rp->ai_socktype,
                    rp->ai_protocol);
        if (fd == -1)
            continue;

        setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &t, sizeof(t));
        if (bind(fd, rp->ai_addr, rp->ai_addrlen) == 0)
            break;

        close(fd);
    }

    if (!rp)
        fd = -1;

    freeaddrinfo(result);

    return fd;
}

/*
 * Send all chunks in one go
 * On success, number of bytes sent is returned
 * on error, -1 returned
 */
static size_t sendall(int fd, char *buf, size_t n)
{
    size_t len, nsent = 0;

    do {
        len = write(fd, buf + nsent, n - nsent);
        if (len == -1)
            break;
        nsent += len;
    } while (nsent < n);

    return len == -1 ? -1 : nsent;
}

static int setnonblocking(int fd)
{
	int flag;

	flag = fcntl(fd, F_GETFL);
	if (flag == -1)
		return -1;
	return fcntl(fd, F_SETFL, O_NONBLOCK);
}

static int handle_request(int sock_fd, struct http_header *hdr,
                          struct server_buf *mem)
{
    char *out_buffer = mem->out_buf;
    int fd, err, status = OK;
    size_t len;

    /* Don't allow access to parent dirs */
    if (strstr(hdr->request_path, ".."))
        status = FORBIDDEN;

    err = filename(out_buffer, BUFSIZE, hdr->request_path);
    if (err == FORBIDDEN || err == -1) {
        fprintf(stderr,
                "could not designate a file name from the request URI: %s\n",
                hdr->request_path);
        return -1;
    }

    if (status == OK) {
        fd = open(out_buffer, O_RDONLY);
        const char *mim = mimestr(hdr->request_path);

        if (fd != -1) {
            len = get_fsize(fd);
            snprintf(out_buffer, BUFSIZE, HEADER_STR,
                     OK, "OK", SRV_FULL_NAME, len,
                     mim);
            sendall(sock_fd, out_buffer, strlen(out_buffer));
        } else {
            status = NOT_FOUND;
        }
    }

    switch (hdr->request_type) {
    case GET:
        if (status == OK) {
            while ((len = read(fd, out_buffer, BUFSIZE)) > 0)
                sendall(sock_fd, out_buffer, len);
        }
        break;
    case HEAD:
        /* Just sent out the header */
        break;
    }

    close(fd);

    return 0;
}

int ssws_init(const char *port, const char *document_root)
{
    struct sockaddr cli_addr;
    size_t size;
    socklen_t cli_sock_len;
    int cli_fd;
    int sock_fd = get_server_fd(port);
    if (sock_fd <= 0)
        return sock_fd;

    errno = 0;
    if (set_document_root(document_root)) {
        fprintf(stderr, "%s: could not set document root, errno: %d - %s\n",
                __func__, errno, strerror(errno));
        return -1;
    }

    if (listen(sock_fd, 12) != 0) {
        printf("listen: %d failed\n", sock_fd);
        close(sock_fd);
        return -1;
    }

    for (;;) {
        cli_fd = accept(sock_fd, &cli_addr, &cli_sock_len);
        if (cli_fd == -1) {
            printf("accept error: %d\n", cli_fd);
            close(sock_fd);
            return -1;
        }

        size = read(cli_fd, app_buf.in_buf, BUFSIZE);

        if (size > 0) {
            app_buf.in_buf[size] = '\0';

            pid_t pid = fork();
            if (pid < 0)
                exit(EXIT_FAILURE);

            /* child process */
            if (pid == 0) {
                close(sock_fd);

                struct http_header header;
                parse_header(&header, app_buf.in_buf, BUFSIZE);
                handle_request(cli_fd, &header, &app_buf);
                printf("header:\n%d, %s, %s\n",
                       header.request_type,
                       header.request_path,
                       header.user_agent);
                exit(EXIT_SUCCESS);
            } else {
                close(cli_fd);
            }
        }
    }
    close(sock_fd);

    return sock_fd;
}
