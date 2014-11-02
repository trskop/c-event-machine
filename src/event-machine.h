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

#ifndef EVENT_MACHINE_H_230071399244842574860511267360184913417
#define EVENT_MACHINE_H_230071399244842574860511267360184913417

#include "event-machine/result.h"
#include <stddef.h>     /* size_t */
#include <stdbool.h>
#include <stdint.h>     /* uint32_t */

#ifdef __cplusplus
extern "C" {
#endif

#define EM_DEFAULT_MAX_EVENTS   4096

/* Forward declaration */
struct EM_s;

typedef void (*EM_event_handler)(struct EM_s *, uint32_t, int, void *);

typedef struct
{
    /* Events as passed to epoll_ctl(). Do not confuse them with events that
     * actually occurred.
     */
    uint32_t events;

    /* File descriptor registered with epoll_ctl(). It is also used as an index
     * for storing pointers to EM_event_descriptor structures.
     */
    int fd;

    /* Private user data are be passed to event handler as is.
     */
    void *data;

    /* Event handler invoked when any registered event occures.
     */
    EM_event_handler handler;
} EM_event_descriptor;

typedef struct
{
    /* Function called by event_machine_add() and event_machine_modify() when
     * registering event descriptor.
     *
     * If this field is NULL then event descriptors aren't stored.
     */
    enum EM_result (*insert)(int, EM_event_descriptor *);

    /* Function called by event_machine_delete() and event_machine_modify()
     * when event descriptor is being unregistered.
     */
    enum EM_result (*remove)(int, EM_event_descriptor **);

    /* Size of private data. See data field documentation for details.
     */
    size_t data_size;

    /* Private data of specific descriptor storage implementation.
     */
    void *data;
} EM_descriptor_storage;

typedef struct EM_s
{
    int epoll_fd;

    /* Pipe used for breaking main processing loop in which epoll_wait() is
     * invoked followed by event processing. See event_machine_run() for
     * details.
     */
    int break_loop_pipe[2];

    /* Event descriptor used for registering read end of pipe used for
     * breaking main processing loop.
     */
    EM_event_descriptor break_loop_event_descriptor;

    /* Function epoll_wait() returns batch of events that occurred. This
     * variable limits maximum number of events processed in one batch and also
     * size of epoll_events array used for storing epoll_wait() results.
     */
    int max_events;

    /* Array used for storing events returned by epoll_wait() function. See
     * also max_events and do_free_epoll_events entries.
     */
    struct epoll_event *epoll_events;

    /* Set to true if epoll_events array was allocated in event_machine_init()
     * and false otherwise.
     */
    bool do_free_epoll_events;

    EM_descriptor_storage descriptor_storage;
} EM;

/* Initialize EM structure.
 *
 * Usage example without user supplied buffer:
 *
 *    // Value EM_DEFAULT_MAX_EVENTS is defined in "event-machine.h".
 *    struct epoll_event epoll_events[EM_DEFAULT_MAX_EVENTS];
 *    EM em = EM_WITH_MAX_EVENTS(EM_DEFAULT_MAX_EVENTS, epoll_events);
 *
 *    if_em_failure (event_machine_init(&em))
 *    {
 *        // Error handling.
 *    }
 *    // ...
 *
 * Usage exampe without user supplied buffer:
 *
 *    EM em = EM_WITH_MAX_EVENTS(EM_DEFAULT_MAX_EVENTS, NULL);
 *    // Same result can be achieved by:
 *    // EM em = EM_DEFAULT;
 *
 *    // Function event_machine_init() will allocate array for storing struct
 *    // epoll_event. Don't forget to call event_machine_destroy() which frees
 *    // that array.
 *    if_em_failure (event_machine_init(&em))
 *    {
 *        // Error handling.
 *    }
 *    // ...
 */
enum EM_result event_machine_init(EM *);

/* Cleanup any resources associated with EM structure.
 */
enum EM_result event_machine_destroy(EM *);

/* Start Event Machine main loop.
 */
enum EM_result event_machine_run(EM *);

/* Notify Event Machine to break main loop as soon as possible.
 */
enum EM_result event_machine_terminate(EM *);

/* Register file descriptor for specified event with its associated data and
 * handler.
 *
 * Do not ever modify event descriptor structure after it was passed to
 * event_machine_add() since it doesn't make any copies and therefore it would
 * cause inconistencies.
 *
 * Usage example:
 *
 *    // ...
 *    EM_event_descriptor ed =
 *    {
 *      .events = EPOLLIN | EPOLLOUT,
 *      .fd = some_fd,
 *      .data = pointer_to_some_private_data,
 *      .handler = pointer_to_event_handling_function
 *    };
 *
 *    if_em_failure (event_machine_add(em, &ed)
 *    {
 *        // Error handling.
 *    }
 *    // ...
 */
enum EM_result event_machine_add(EM *, EM_event_descriptor *);

/* Unregister file descriptor.
 *
 * If descriptor storage is initialized then this function returns
 * EM_event_descriptor associated with specified file descriptor. It's the same
 * value (pointer) as it was passed to event_machine_add(). It is up to the
 * caller to free the memory if needed. If last argument is NULL, then function
 * simply makes event machine forget that any such EM_event_descriptor instance
 * existed. Do this only if there is other way how that memory can be freed or
 * if it was allocated statically or on stack.
 *
 * In case that descriptor storage is not initialized it always returns NULL if
 * caller supplies non-NULL pointer.
 */
enum EM_result event_machine_delete(EM *, int, EM_event_descriptor **);

/* Similar as calling event_machine_delete() followed by event_machine_add(),
 * but uses only one epoll_ctl() call.
 *
 * Event descriptor passed to this function should be different from the one
 * used by previous event_machine_add(), because event descriptor may not be
 * changed while it is registered. Function returns event descriptor that was
 * used during event_machine_add() call as does event_machine_delete(). See
 * event_machine_delete() for details.
 */
enum EM_result event_machine_modify(EM *, int, EM_event_descriptor *,
    EM_event_descriptor **);

/* Statically set user specified entries of EM structure.
 *
 * Usage example:
 *
 *    struct epoll_event epoll_events[SOME_SIZE];
 *    EM em = EM_STATIC_WITH_MAX_EVENTS(SOME_SIZE, epoll_events);
 *    // ...
 *
 *    if_em_failure (event_machine_init(&em))
 *    {
 *        // Error handling.
 *    }
 *    // ...
 */
#define EM_STATIC_WITH_MAX_EVENTS(maxevs, evs)  \
    { .epoll_fd = -1                            \
    , .break_loop_pipe = {-1, -1}               \
    , .break_loop_event_descriptor =            \
        { .events = 0                           \
        , .fd = -1                              \
        , .data = NULL                          \
        , .handler = NULL                       \
        }                                       \
    , .max_events = maxevs                      \
    , .epoll_events = evs                       \
    , .descriptor_storage =                     \
        { .insert = NULL                        \
        , .remove = NULL                        \
        , .data_size = 0                        \
        , .data = NULL                          \
        }                                       \
    }

/* Statically set user specified entries of EM structure to their default
 * values.
 *
 * Usage example:
 *
 *    EM em = EM_STATIC_DEFAULT;
 *    // ...
 *
 *    // Function event_machine_init() will allocate array for storing struct
 *    // epoll_event. Don't forget to call event_machine_destroy() which frees
 *    // that array.
 *    if_em_failure (event_machine_init(&em))
 *    {
 *        // Error handling.
 *    }
 *    // ...
 */
#define EM_STATIC_DEFAULT   \
    EM_STATIC_WITH_MAX_EVENTS(EM_DEFAULT_MAX_EVENTS, NULL)

#ifdef __cplusplus
}
#endif

#endif /* EVENT_MACHINE_H_230071399244842574860511267360184913417 */
