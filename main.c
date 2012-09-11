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

#include <signal.h>

#include <stdio.h>
#include <stdlib.h>

#include "ssws_core.h"

#define error_exit(_msg, _arg...)       \
    do {                                \
        fprintf(stderr, _msg, ##_arg);  \
        exit(EXIT_FAILURE);             \
    } while (0)

int main(int argc, char **argv)
{
    unsigned short int port;

    if (argc < 3)
        error_exit("Usage:\n\t%s port document_root\n"
                   "Example:\n\t%s 33333 /var/www/html\n\n",
                   argv[0], argv[0]);

    port = atoi(argv[1]);
    if (port < 1 || port > 65535)
        error_exit("Invalid port number\n");

    /* Ignore child process state changes */
    signal(SIGCHLD, SIG_IGN);
    signal(SIGHUP, SIG_IGN);

    if (ssws_init(argv[1], NULL) == -1)
        error_exit("ssws_init failed!\n");

    printf("Whooaa:\n");

    return 0;
}
