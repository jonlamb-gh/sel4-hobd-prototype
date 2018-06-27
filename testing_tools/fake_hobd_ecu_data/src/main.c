/**
 * @file main.c
 * @brief TODO.
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <assert.h>
#include <popt.h>

#define TCP_PORT (1235)
#define TCP_ADDR "127.0.0.1"

int main(int argc, char **argv)
{
    int err = 0;
    struct sockaddr_in server_address;

    memset(&server_address, 0, sizeof(server_address));

	server_address.sin_family = AF_INET;
    server_address.sin_port = htons(TCP_PORT);
    err = inet_pton(AF_INET, TCP_ADDR, &server_address.sin_addr);
    assert(err == 1);

    const int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    assert(socket_fd >= 0);

    err = connect(
            socket_fd,
            (struct sockaddr*) &server_address,
            sizeof(server_address));
    if(err != 0)
    {
        fprintf(stderr, "failed to connect to %s:%u\n", TCP_ADDR, TCP_PORT);
        assert(err == 0);
    }

    // TODO
    // send(socket_fs, data, size, flags);

    if(socket_fd >= 0)
    {
        (void) close(socket_fd);
    }

    if(err == 0)
    {
        err = EXIT_SUCCESS;
    }
    else
    {
        err = EXIT_FAILURE;
    }

    return err;
}
