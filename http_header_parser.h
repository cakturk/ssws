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

#ifndef HTTP_HEADER_PARSER_H
#define HTTP_HEADER_PARSER_H

#define BUFSIZE 4096

/* Request types eg..(GET, HEAD..) */
#define GET 0
#define HEAD 1
#define NOT_SUPPORTED 2
#define OK 200
#define BAD_REQUEST 400
#define FORBIDDEN 403
#define NOT_FOUND 404

#define SRV_NAME "chngr's ssws"
#define SRV_VERSION "0.1"
#define SRV_FULL_NAME SRV_NAME "/" SRV_VERSION

#define INDEX "index.html"

#define HEADER_STR \
    "HTTP/1.1 %d %s\n"          \
    "Server: %s\n"              \
    "Content-Length: %zu\n"     \
    "Connection: close\n"       \
    "Content-Type: %s\n\n"

struct http_header {
    int request_type;
    char *request_path;
    char *host;
    char *user_agent;
};

extern int set_document_root(const char *docroot);
extern int filename(char *buf, size_t size, const char *request_path);
extern int parse_header(struct http_header *hdr,
                        char *hdr_data,
                        size_t size);

extern const char *mimestr(const char *file_name);

#endif /* end of include guard: HTTP_HEADER_PARSER_H */

