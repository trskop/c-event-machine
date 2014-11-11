#include "event-machine.h"
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <unistd.h>


void stdin_handler(EM *em, uint32_t events, int fd, void *data)
{
    char buffer[4096];
    ssize_t len;

    if ((len = read(fd, &buffer, 4096)) < 0)
    {
        // Reading from standard input failed.
        perror("stdin_handler(): read()");
    }
    else if (len == 0)
    {
        // Standard input was closed.
        ;
    }
    else if (write(STDOUT_FILENO, &buffer, len) < 0)
    {
        // Writing to standard output failed.
        perror("stdin_handler(): write()");
    }
    else
    {
        // Success.
        return;
    }

    // Terminate event handling loop as soon as it processes all enqueued
    // events.
    event_machine_terminate(em);

    return;
}

int main()
{
    // Create static memory for event machine.
    //
    // This way the event machine desn't need to allocate memory in
    // event_machine_init().
    struct epoll_event epoll_events[EM_DEFAULT_MAX_EVENTS];
    EM em = EM_STATIC_WITH_MAX_EVENTS(EM_DEFAULT_MAX_EVENTS, epoll_events);

    // Initialize event machine structure.
    //
    // Don't ever use event machine without initializing it first, it won't
    // work.
    if_em_failure (event_machine_init(&em))
    {
        exit(EXIT_FAILURE);
    }

    // Create event descriptor structure and set which events should trigger
    // callback.
    //
    // Event descriptor will watch STDIN_FILENO file descriptor and execute
    // stdin_handler callback upon receiving incoming data on stdin.
    //
    // For list of acceptable events see EM_event_descriptor documentation or
    // epoll(7) manual page.
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

    // Run event machine.
    //
    // This executes event handling loop that is interruptible, as demonstrates
    // this example, by calling event_machine_terminate().
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
