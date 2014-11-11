#include "event-machine.h"
#include "event-timer.h"
#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <stdio.h>


// Explained in stdin-stdout.c.
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

// Timeout callback for periodic timer.
void timer_timeout(Event_timer *timer, void *data)
{
    printf("timer timeout\n");
}

// Timeout callback for one shot timer.
void oneshot_timer_timeout(Event_timer *timer, void *data)
{
    printf("timer oneshot timeout\n");
}

int main()
{
    struct epoll_event epoll_events[EM_DEFAULT_MAX_EVENTS];
    EM em = EM_STATIC_WITH_MAX_EVENTS(EM_DEFAULT_MAX_EVENTS, epoll_events);

    // Event machine initialization must be executed before timer_create().
    if_em_failure (event_machine_init(&em))
    {
        exit(EXIT_FAILURE);
    }

    // This part is inherited from stdin-stdout.c to make example 
    // interruptible. This is not important for event-timer.
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

    // Create structure for timer. This structure must stay allocated for the 
    // timer life time. Parameters passed to event_timer_create() are event 
    // machie, timer, callback timer_timeout(), NULL.
    Event_timer timer;
    if (is_em_failure(event_timer_create(&em, &timer, timer_timeout, NULL)))
    {
        exit(EXIT_FAILURE);
    }
    // Start timer. Parameters are timer structure, timeout in ms and also
    // false to set timer to periodic state.
    if (is_em_failure(event_timer_start(&timer, 1500, false)))
    {
        exit(EXIT_FAILURE);
    }

    // Again create and start timer with one difference: timer is one shot.
    // That means timer will expirate only once.
    Event_timer oneshot_timer;
    if (is_em_failure(event_timer_create(&em, &oneshot_timer,
        oneshot_timer_timeout, NULL)))
    {
        exit(EXIT_FAILURE);
    }
    if (is_em_failure(event_timer_start(&oneshot_timer, 1500, true)))
    {
        exit(EXIT_FAILURE);
    }

    // And run event machine to process all event coming from timers.
    if_em_failure (event_machine_run(&em))
    {
        exit(EXIT_FAILURE);
    }

    // Stop periodic timer.
    // Oneshot timer don't need to be stopped because it should be stopped
    // by now on its own.
    if (is_em_failure(event_timer_stop(&timer)))
    {
        exit(EXIT_FAILURE);
    }

    // Destroy both timers.
    if (is_em_failure(event_timer_destroy(&timer)))
    {
        exit(EXIT_FAILURE);
    }
    if (is_em_failure(event_timer_destroy(&oneshot_timer)))
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
