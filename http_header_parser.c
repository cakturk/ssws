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

#include <sys/types.h>
#include <sys/stat.h>

#include <limits.h> /* PATH_MAX */

#include <stdlib.h> /* malloc, realpath */
#include <string.h>
#include <stdio.h>

#include "http_header_parser.h"

static char *doc_root;

int set_document_root(const char *docroot)
{
	struct stat sb;
	char abs_path[PATH_MAX];
	int err;

	if (!docroot || docroot[0] == '\0')
		return -1;

	err = stat(docroot, &sb);
	if (err || !S_ISDIR(sb.st_mode))
		return -1;

	if (!realpath(docroot, abs_path))
		return -1;

	doc_root = strdup(abs_path);

	printf("set document root to %s\n", doc_root);

	return 0;
}

struct request_metod {
	const char *name;
	int value;
};

static const struct request_metod supported_methods[] = {
	{ "GET" , GET  },
	{ "HEAD", HEAD },
	{ NULL  , 0    }
};

struct mime_type {
	const char *ext;
	const char *file_type;
};

static const struct mime_type supported_mime_types[] = {
	{ "html", "text/html"         },
	{ "htm" , "text/html"         },
	{ "jpg" , "image/jpg"         },
	{ "jpeg", "image/jpeg"        },
	{ "png" , "image/png"         },
	{ "gif" , "image/gif"         },
	{ "tgz" , "application/x-gz"  },
	{ "gz"  , "application/x-gz"  },
	{ "tar" , "application/x-tar" },
	{ "zip" , "application/zip"   },
	{ NULL  , NULL                }
};

struct server_data {
	struct http_header *header;
	char *payload;
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

int parse_header(struct http_header *hdr,
		 char *hdr_data,
		 size_t size)
{
	const struct request_metod *method;
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


inline char *getfileext(const char *fname)
{
	char *ret = strrchr(fname, '.');
	if (ret)
		ret = ret + 1;

	return ret;
}

int filename(char *buf, size_t size, const char *request_path)
{
	struct stat sb;
	size_t len;
	int isdir = 0;

	if (strstr(request_path, ".."))
		return FORBIDDEN;

	if (snprintf(buf, size, "%s/%s", doc_root, request_path) >= size)
		return -1;

	if (stat(buf, &sb) == 0)
		isdir = S_ISDIR(sb.st_mode);

	len = strlen(buf);

	if (isdir) {
		const char *dir_index;

		if (len > 0 && *(request_path + len - 1) == '/')
			dir_index = "index.html";
		else
			dir_index = "/index.html";
		if (snprintf(buf, size, "%s%s%s",
			     doc_root,
			     request_path,
			     dir_index) >= size)
			return -1;
	} else {
		if (snprintf(buf, size, "%s/%s", doc_root, request_path) >= size)
			return -1;
	}

	return OK;
}

const char *mimestr(const char *file_name)
{
	const struct mime_type *m = supported_mime_types;
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
