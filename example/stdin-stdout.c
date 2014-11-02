#include "event-machine.h"
#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <unistd.h>


void stdin_handler(EM *em, uint32_t events, int fd, void *data)
{
    char buffer[4096];
    ssize_t len;

    if ((len = read(fd, &buffer, 4096)) < 0)
    {
        ;
    }
    else if (len == 0)
    {
        ;
    }
    else if (write(STDOUT_FILENO, &buffer, len) < 0)
    {
        ;
    }
    else
    {
        return;
    }

    event_machine_terminate(em);

    return;
}

int main()
{
    struct epoll_event epoll_events[EM_DEFAULT_MAX_EVENTS];
    EM em = EM_STATIC_WITH_MAX_EVENTS(EM_DEFAULT_MAX_EVENTS, epoll_events);

    if_em_failure (event_machine_init(&em))
    {
        exit(EXIT_FAILURE);
    }

    EM_event_descriptor ed =
        { .events = EPOLLIN
        , .fd = STDIN_FILENO
        , .data = NULL
        , .handler = stdin_handler
        };
    if_em_failure (event_machine_add(&em, &ed))
    {
        exit(EXIT_FAILURE);
    }

    if_em_failure (event_machine_run(&em))
    {
        exit(EXIT_FAILURE);
    }

    if_em_failure (event_machine_destroy(&em))
    {
        exit(EXIT_FAILURE);
    }

    exit(EXIT_SUCCESS);
}
