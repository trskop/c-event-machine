/* Copyright (c) 2014, Peter Trško <peter.trsko@gmail.com>
 * Copyright (c) 2014, Jan Šipr <sipr.jan@gmail.com>
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *
 *     * Neither the name of Peter Trško nor the names of other
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "event-machine.h"
#include "event-timer.h"
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <unistd.h>


// Explained in stdin-stdout.c example.
void stdin_handler(EM *em, uint32_t events, int fd, void *data)
{
    char buffer[4096];
    ssize_t len;

    if ((len = read(fd, &buffer, 4096)) < 0)
    {
        perror("stdin_handler(): read()");
    }
    else if (len == 0)
    {
        ;
    }
    else if (write(STDOUT_FILENO, &buffer, len) < 0)
    {
        perror("stdin_handler(): write()");
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
    // interruptible. This is not important for event-timer. See
    // stdin-stdout.c example for details.
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

    // Create Event_timer structure for one specific timer instance.
    //
    // This structure may not be for more then one timer at a time and stay
    // allocated as long as the timer is used. Only after event_timer_destroy()
    // is called it may be deallocated. However the memory it hodls may be
    // reused after event_timer_destroy() call.
    //
    // Parameters passed to event_timer_create() are:
    //
    // 1. Event machie, which willwatch for timer expiration events.
    // 2. Timer, structure that describes specific timer instance.
    // 3. callback, in this case timer_timeout(), its execution is triggered by
    //    event machine when it detects timer expiration.
    // 4. NULL, pointer to private data passed to callback when its executed.
    //    In this case "NULL = nothing".
    Event_timer timer;
    if (is_em_failure(event_timer_create(&em, &timer, timer_timeout, NULL)))
    {
        exit(EXIT_FAILURE);
    }

    // Start timer.
    //
    // Parameters are:
    //
    // 1. Timer structure, see comment above.
    // 2. Expiration time (i.e. timeout) in milliseconds
    // 3. Boolean value that indicates if timer is periodic or one-shot. In
    //    this case its value is false which means periodic timer.
    if (is_em_failure(event_timer_start(&timer, 1500, false)))
    {
        exit(EXIT_FAILURE);
    }

    // Again create and start timer with one difference: this timer is one-shot
    // timer. It means that this timer will expirate only once.
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
    //
    // Oneshot timer doesn't need to be stopped because it should be stopped
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
