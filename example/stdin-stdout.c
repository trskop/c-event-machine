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
    // Create static memory for event machine.
    // This way the event machine don't need to allocate memory.
    struct epoll_event epoll_events[EM_DEFAULT_MAX_EVENTS];
    EM em = EM_STATIC_WITH_MAX_EVENTS(EM_DEFAULT_MAX_EVENTS, epoll_events);

    // Initialize event machine structure.
    if_em_failure (event_machine_init(&em))
    {
        exit(EXIT_FAILURE);
    }

    // Create event descriptor structure and set which event should trigger
    // callback. For events acceptable see EM_event_descriptor documentation.
    // Event descriptor will watch STDIN_FILENO file descriptor and execute
    // stdin_handler handle upon receiving incoming data on stdin.
    EM_event_descriptor ed =
        { .events = EPOLLIN
        , .fd = STDIN_FILENO
        , .data = NULL
        , .handler = stdin_handler
        };
    // Register event in event machine.
    if_em_failure (event_machine_add(&em, &ed))
    {
        exit(EXIT_FAILURE);
    }

    // Run event machine. Infinite loop is inside.
    if_em_failure (event_machine_run(&em))
    {
        exit(EXIT_FAILURE);
    }

    // Destroy event machine.
    if_em_failure (event_machine_destroy(&em))
    {
        exit(EXIT_FAILURE);
    }

    exit(EXIT_SUCCESS);
}
