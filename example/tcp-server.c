#include "event-machine.h"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>


struct connection_data
{
    /* Pointer that will referre to struct "accept_connection_data" that this
     * is part of. It will be used to free the whole data structure.
     */
    void *pointer;
    struct sockaddr_in remote_address;
};

struct accept_connection_data
{
    EM_event_descriptor event_descriptor;
    struct connection_data data;
};

void connection_hadnler(EM *em, uint32_t events, int socket, void *data)
{
    struct sockaddr_in remote_address =
        ((struct connection_data *)data)->remote_address;
    char read_buffer[BUFSIZ];
    ssize_t read_len;

    if (events & EPOLLIN)
    {
        memset(read_buffer, 0, BUFSIZ);
        read_len = read(socket, &read_buffer, BUFSIZ);
        if (read_len < 0)
        {
            perror("read");
            return;
        }
        else if (read_len > 0)
        {
            printf("%s: %s", inet_ntoa(remote_address.sin_addr), read_buffer);
        }
    }

    /* Error detected or remote site disconnected.
     */
    if (events & EPOLLERR || events & EPOLLRDHUP)
    {
        EM_event_descriptor *old_ed;

        if_em_failure (event_machine_delete(em, socket, &old_ed))
        {
            // TODO print error
            ;
        }
        close(socket);

        /* event_machine_delete() may return NULL if descriptor storage isn't
         * used or if it doesn't support remove operation.
         */
        if (old_ed != NULL)
        {
            free(((struct connection_data *)old_ed->data)->pointer);
        }

        printf("%s: *** Closed connection. ***\n",
            inet_ntoa(remote_address.sin_addr));
    }
}

void accept_handler(EM *em, uint32_t events, int listening_socket, void *data)
{
    int socket;
    socklen_t remote_address_len = sizeof(struct sockaddr_in);
    struct sockaddr *remote_address;
    struct accept_connection_data *accept_data;
    EM_event_descriptor *ed;

    accept_data = malloc(sizeof(struct accept_connection_data));
    if (accept_data == NULL)
    {
        // TODO: Proper error handling.
        perror("malloc");
        return;
    }
    accept_data->data.pointer = (void *)accept_data;
    ed = &(accept_data->event_descriptor);
    remote_address = (struct sockaddr *)&(accept_data->data.remote_address);

    socket = accept(listening_socket, remote_address, &remote_address_len);
    if (socket < 0)
    {
        // TODO: Proper error handling.
        perror("accept");
        return;
    }

    ed->events = EPOLLIN | EPOLLRDHUP | EPOLLET;
    ed->fd = socket;
    ed->data = &(accept_data->data);
    ed->handler = connection_hadnler;
    if_em_failure (event_machine_add(em, ed))
    {
        // TODO: Proper error handling.
        printf("event_machine_add(): failed.\n");
        return;
    }

    printf("%s: *** Accepted connection. ***\n",
        inet_ntoa(accept_data->data.remote_address.sin_addr));
}

int main()
{
    int listening_socket;

    const size_t max_epoll_events = EM_DEFAULT_MAX_EVENTS;
    struct epoll_event epoll_events[max_epoll_events];
    EM em = EM_STATIC_WITH_MAX_EVENTS(max_epoll_events, epoll_events);

    listening_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (listening_socket < 0)
    {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in listening_address =
        { .sin_family = AF_INET
        , .sin_port = htons((uint16_t)4040)
        , .sin_addr.s_addr = inet_addr("127.0.0.1")
        };
    if (bind(listening_socket, (struct sockaddr *)&listening_address,
        (socklen_t)sizeof(struct sockaddr_in)))
    {
        perror("bind");
        exit(EXIT_FAILURE);
    }

    if (listen(listening_socket, 128) < 0)
    {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    if_em_failure (event_machine_init(&em))
    {
        exit(EXIT_FAILURE);
    }

    EM_event_descriptor ed =
        { .events = EPOLLIN
        , .fd = listening_socket
        , .data = NULL
        , .handler = accept_handler
        };
    if_em_failure (event_machine_add(&em, &ed))
    {
        exit(EXIT_FAILURE);
    }

    if_em_failure (event_machine_run(&em))
    {
        exit(EXIT_FAILURE);
    }

    /* {{{ Cleanup ********************************************************* */

    if_em_failure (event_machine_delete(&em, listening_socket, NULL))
    {
        exit(EXIT_FAILURE);
    }
    if_em_failure (event_machine_destroy(&em))
    {
        exit(EXIT_FAILURE);
    }
    if (close(listening_socket) != 0)
    {
        perror("close");
    }

    /* }}} Cleanup ********************************************************* */

    exit(EXIT_SUCCESS);
}
