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

#include "ecu.h"

#define TCP_PORT (1235)
#define TCP_ADDR "127.0.0.1"

static volatile sig_atomic_t g_exit_signaled;

static void sig_handler(
        int sig)
{
    if(sig == SIGINT)
    {
        g_exit_signaled = 1;
    }
}

int main(
        int argc,
        char **argv)
{
    int err = 0;
    struct sockaddr_in server_address;
    struct sigaction sigact;

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

    (void) memset(&sigact, 0, sizeof(sigact));
    g_exit_signaled = 0;
    sigact.sa_flags = SA_RESTART;
    sigact.sa_handler = sig_handler;

    err = sigaction(SIGINT, &sigact, 0);
    assert(err == 0);

    ecu_s * const ecu = ecu_new(socket_fd);
    assert(ecu != NULL);

    while(g_exit_signaled == 0)
    {
        err = ecu_update(ecu);

        if(err != 0)
        {
            g_exit_signaled = 1;
        }
    }

    if(ecu != NULL)
    {
        free(ecu);
    }

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
