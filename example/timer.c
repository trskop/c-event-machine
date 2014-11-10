#include "event-machine.h"
#include "event-timer.h"
#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <stdio.h>


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

void timer_timeout(Event_timer *timer, void *data)
{
    printf("timer timeout\n");
}

void oneshot_timer_timeout(Event_timer *timer, void *data)
{
    printf("timer oneshot timeout\n");
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

    Event_timer timer;
    if (is_em_failure(event_timer_create(&em, &timer, timer_timeout, NULL))
    || is_em_failure(event_timer_start(&timer, 1500, false)))
    {
    	perror("error: ");
        exit(EXIT_FAILURE);
    }

    Event_timer oneshot_timer;
    if(is_em_failure(event_timer_create(&em, &oneshot_timer, oneshot_timer_timeout, NULL))
    || is_em_failure(event_timer_start(&oneshot_timer, 1500, true)))
    {
        perror("error: ");
        exit(EXIT_FAILURE);
    }

    if_em_failure (event_machine_run(&em))
    {
        exit(EXIT_FAILURE);
    }

    if (is_em_failure(event_timer_stop(&timer))
        || is_em_failure(event_timer_destroy(&timer))
        || is_em_failure(event_timer_destroy(&oneshot_timer)))
    {
        exit(EXIT_FAILURE);
    }

    if_em_failure (event_machine_destroy(&em))
    {
        exit(EXIT_FAILURE);
    }

    exit(EXIT_SUCCESS);
}
