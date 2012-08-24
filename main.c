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
        error_exit("Usage: %s port document_root\n", argv[0]);

    port = atoi(argv[1]);
    if (port < 1 || port > 65535)
        error_exit("Invalid port number\n");

    printf("Whooaa: %d\n", ssws_init());

    return 0;
}