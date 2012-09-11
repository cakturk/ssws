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

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h> /* former <arpa/inet.h>*/
#include <netdb.h>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "http_header_parser.h"

#define BUFSIZE 4096

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


static int handle_request(int sock_fd, struct http_header *hdr)
{
    static char out_buffer[BUFSIZE];
    int fd, status = OK;
    size_t len;
    /* define MAXMIMELEN for convenience */
    char buf[32];

    /* Don't allow access to parent dirs */
    if (strstr(hdr->request_path, ".."))
        status = FORBIDDEN;

    filename(buf, 32, hdr->request_path);

    if (status == OK) {
        fd = open(buf, O_RDONLY);
        const char *mim = mimestr(hdr->request_path);

        if (fd != -1) {
            len = get_fsize(fd);
            snprintf(out_buffer, BUFSIZE, HEADER_STR,
                     OK, "OK", SRV_FULL_NAME, len,
                     mim);
            write(sock_fd, out_buffer, strlen(out_buffer));
        } else {
            status = NOT_FOUND;
        }
    }

    switch (hdr->request_type) {
    case GET:
        if (status == OK) {
            while ((len = read(fd, out_buffer, BUFSIZE)) > 0)
                write(sock_fd, out_buffer, len);
        }
        break;
    case HEAD:
        /* Just sent out the header */
        break;
    }

    close(fd);

    return 0;
}

int ssws_init(const char *port, const char *doc_root)
{
    char buf[BUFSIZE];
    struct sockaddr cli_addr;
    size_t size;
    socklen_t cli_sock_len;
    int cli_fd;
    int sock_fd = get_server_fd(port);
    if (sock_fd <= 0)
        return sock_fd;


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
        //printf("Got connection!\n");
        size = read(cli_fd, buf, BUFSIZE);

        if (size > 0) {
            buf[size] = '\0';

            pid_t pid = fork();
            if (pid < 0)
                exit(EXIT_FAILURE);

            /* child process */
            if (pid == 0) {
                close(sock_fd);

                struct http_header header;
                parse_header(&header, buf, BUFSIZ);
                handle_request(cli_fd, &header);
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
