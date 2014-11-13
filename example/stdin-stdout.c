/* Copyright (c) 2014, Peter Trško <peter.trsko@gmail.com>
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
