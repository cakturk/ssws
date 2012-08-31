#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <string.h>
#include <stdio.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>

#define BUFSIZE 4096

/* Request types eg(GET, HEAD..) */
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

#define HEADER_STR \
    "HTTP/1.1 %d %s\n"          \
    "Server: %s\n"              \
    "Content-Length: %zu\n"     \
    "Connection: close\n"       \
    "Content-Type: %s\n\n"

struct request_metod {
    char *name;
    int   value;
};

static struct request_metod supported_methods[] = {
    { "GET" , GET  },
    { "HEAD", HEAD },
    { NULL  , 0    }
};

struct mime_type {
    char *ext;
    char *file_type;
};

static struct mime_type supported_mime_types[] = {
    { "html", "text/html" },
    { "htm" , "text/html" },
    { "jpg" , "image/jpg" },
    { "jpeg", "image/jpeg"},
    { "png" , "image/png" },
    { "gif" , "image/gif" },
    { "zip" , "image/zip" },
    { "gz"  , "image/gz"  },
    { "tar" , "image/tar" },
    { NULL  , NULL        }
};

struct http_header {
    int request_type;
    char *request_path;
    char *host;
    char *user_agent;
};

static char *get_next_line(char **next, char *end)
{
    char *str = *next;
    char *result = str;

    if (!str || *(str+1) == '\n')
        return NULL;

    *next = NULL;

    for (; str < end; ++str) {
        if (*str == '\n') {
            *str = '\0';
            if (str + 1 < end && *(str + 1) != '\0')
                *next = str + 1;
            break;
        }
    }

    return result;
}

static int parse_header(struct http_header *hdr,
                        char *hdr_data,
                        size_t size)
{
    struct request_metod *method;
    char *i, *next, *end;
    int result = -1;

    end = hdr_data + size;
    next = hdr_data;

    i = get_next_line(&next, end);
    if (!i)
        goto out;
    if((i = strtok(i, " ")) == NULL)
        goto out;
    for (method = supported_methods; method != NULL; ++method) {
        if (strncmp(i, method->name, strlen(method->name)) == 0) {
            hdr->request_type = method->value;
            break;
        }
    }

    if((i = strtok(NULL, " ")) == NULL)
        goto out;
    hdr->request_path = i + 1;
    /* We got enough header data */
    result = 0;

    if ((i = get_next_line(&next, end)) == NULL)
        goto out;
    if((i = strtok(i, " ")) == NULL)
        goto out;
    if (strncmp(i, "Host:", strlen("Host:")) == 0) {
        i = strtok(NULL, " ");
        hdr->host = i;
    }

    while ((i = get_next_line(&next, end))) {
        char *token = strtok(i, " ");
        int found = 0;
        while (token) {
            if (strncmp(token, "User-Agent:",
                        strlen("User-Agent:")) == 0)
                found = 1;

            if (found) {
                hdr->user_agent = token + strlen(token) + 1;
                goto out;
            } else {
                token = strtok(NULL, " ");
            }
        }
    }

out:
    return result;
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

static inline size_t get_fsize(int fd)
{
    size_t len = (size_t)lseek(fd, 0, SEEK_END);
    lseek(fd, 0, SEEK_SET);
    return len;
}

static inline char *getfileext(char *fname)
{
    char *ret = strrchr(fname, '.');
    if (ret)
        ret = ret + 1;

    return ret;
}

static char *mimestr(char *file_name)
{
    struct mime_type *m = supported_mime_types;
    char *ext = getfileext(file_name);
    size_t len;

    if (ext) {
        len = strlen(ext);
        for (; m->ext != NULL; ++m)
            if (strncmp(m->ext, ext, len) == 0)
                return m->file_type;
    }

    return NULL;
}

static int handle_request(int sock_fd, struct http_header *hdr)
{
    static char out_buffer[BUFSIZE];
    int fd, status = OK;
    size_t len;

    /* Don't allow access to parent dirs */
    if (strstr(hdr->request_path, ".."))
        status = FORBIDDEN;

    if (status == OK) {
        char *mime = mimestr(hdr->request_path);
        fd = open(hdr->request_path, O_RDONLY);
        if (fd != -1 && mime) {
            len = get_fsize(fd);
            snprintf(out_buffer, BUFSIZE, HEADER_STR,
                     OK, "OK", SRV_FULL_NAME, len,
                     mime);
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
        printf("Got connection!\n");
        size = read(cli_fd, buf, BUFSIZE);

        if (size > 0) {
            buf[size] = '\0';
            printf("Date from wire:\n%s\n", buf);
            struct http_header header;
            parse_header(&header, buf, BUFSIZ);
            handle_request(cli_fd, &header);
            printf("heaa: %d, %s, %s\n",
                   header.request_type,
                   header.request_path,
                   header.user_agent);
        }
        close(cli_fd);
    }
    close(sock_fd);

    return sock_fd;
}
