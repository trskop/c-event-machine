/* Copyright (c) 2014, 2015, Peter Trško <peter.trsko@gmail.com>
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

/** @file event-machine.h
 *
 * Simple low-level event machine based on Linux <tt>epoll</tt> or FreeBSD
 * <tt>kqueue</tt>.
 *
 * Usage examples can be found here:
 *
 * @li @link example/stdin-stdout.c @endlink
 * @li @link example/tcp-server.c @endlink
 *
 * @author Peter Trško
 * @date 2014
 * @copyright BSD3
 *
 * @example example/stdin-stdout.c
 *   Behaves as simple version of UNIX <tt>cat</tt> command that reads from
 *   <tt>stdin</tt> and writes to <tt>stdout</tt>. Note that input may not be a
 *   file, because file descriptors for regular files can not be
 *   <tt>epoll()</tt>-ed, it would not make sense. Example can be properly
 *   terminated by closing standard input (<tt>CTRL-D</tt> on POSIX-like
 *   platforms). For details see mentioned
 *   @link example/stdin-stdout.c @endlink.
 *
 * @example example/tcp-server.c
 *   Simple TCP server that reads data from clients and prints it to stdout.
 *   For details see mentioned @link example/stdin-stdout.c @endlink.
 */

#ifndef EVENT_MACHINE_H_230071399244842574860511267360184913417
#define EVENT_MACHINE_H_230071399244842574860511267360184913417

#if defined(USE_EPOLL) && defined(USE_KQUEUE)
#error USE_EPOLL and USE_KQUEUE are mutually exculisive.
#endif

#include "event-machine/result.h"
#include <stddef.h>     /* size_t */
#include <stdbool.h>
#include <stdint.h>     /* uint32_t */

/* {{{ Platform Dependent Imports ********************************************/

#ifdef USE_EPOLL
#include <sys/epoll.h>
#endif /* HAVE_EPOLL */

#if USE_KQUEUE
#include <sys/event.h>
#endif /* HAVE_KQUEUE */

/* }}} Platform Dependent Imports ********************************************/

#ifdef __cplusplus
extern "C" {
#endif

/* {{{ Platform Dependent Code ***********************************************/

#if USE_EPOLL
#define EVENT_READ  EPOLLIN
#define EVENT_WRITE EPOLLOUT

typedef struct epoll_event event_t;
typedef uint32_t event_filter_t;
#endif /* USE_EPOLL */

#if USE_KQUEUE
#define EVENT_READ  EVFILT_READ
#define EVENT_WRITE EVFILT_WRITE

typedef struct kevent event_t;
typedef int16_t event_filter_t;
#endif /* USE_KQUEUE */

/* }}} Platform Dependent Code ***********************************************/

/** Default value of how many events should event machine process in one
 * iteration.
 */
#define EM_DEFAULT_MAX_EVENTS   4096

struct EM_s;    /* Forward declaration */

/** Type of callbacks triggered by event.
 *
 * @param[in] event_machine
 *   #EM (event machine) that invoked this callback function.
 *
 * @param[in] events
 *   Bitarray of events that triggered this specific call of callback function.
 *
 * @param[in] fd
 *   File descriptor for which events occurred while being monitored by
 *   event_machine.
 *
 * @param[in] data
 *   Pointer to private data associated with this specific file descriptor
 *   via EM_event_descriptor.
 */
typedef void (*EM_event_handler)(struct EM_s *event_machine, uint32_t events,
    int fd, void *data);

/** Structure that describes set of events that event machine monitors on
 * specific file descriptor.
 *
 * Instances of this structure has to be registered in an #EM (event machine)
 * using event_machine_add() function. Example:
 *
 * @code{.c}
 * // ...
 * EM_event_descriptor event_descriptor =
 * {
 *     // Listen for data ready for read and do it in edge-triggered mode.
 *     // See epoll(7) manual page for details.
 *     .events = EPOLLIN | EPOLLET,
 *
 *     // File descriptor that will be monitored by event machine for above
 *     // specified events.
 *     .fd = some_file_descriptor,
 *
 *     // Pointer that will be passed to each incarnation of "handler"
 *     // callback.
 *     .data = some_state
 *
 *     // Function called each time one of the above events is detected on
 *     // above file descriptor.
 *     .handler = some_function
 * }
 *
 * if (event_machine_add(event_machine, &event_descriptor) != EM_SUCCESS)
 * {
 *     // Error handling.
 * }
 * // ...
 * @endcode
 */
typedef struct
{
    /** Events as passed to epoll_ctl(). Do not confuse them with events that
     * actually occurred.
     */
    event_filter_t events;

    /** File descriptor registered with epoll_ctl(). It is also used as an
     * index for storing pointers to EM_event_descriptor structures.
     */
    int fd;

    /** Private user data are be passed to event handler as is.
     *
     * Value may be <tt>NULL</tt>
     */
    void *data;

    /** Event handler invoked when any registered event occures.
     */
    EM_event_handler handler;
} EM_event_descriptor;

typedef struct
{
    /** Function called by <tt>event_machine_add()</tt> and
     * <tt>event_machine_modify()</tt> when registering event descriptor.
     *
     * If this field is <tt>NULL</tt> then event descriptors aren't stored.
     */
    uint32_t (*insert)(int, EM_event_descriptor *);

    /** Function called by <tt>event_machine_delete()</tt> and
     * <tt>event_machine_modify()</tt> when event descriptor is being
     * unregistered.
     *
     * If this field is <tt>NULL</tt>, then event descriptors can't be
     * retrieved again. It is perfectly find to define <tt>insert</tt>, but not
     * <tt>remove</tt>.
     */
    uint32_t (*remove)(int, EM_event_descriptor **);

    /** Size of private data. See data field documentation for details.
     *
     * If value of <tt>data = NULL</tt> then this value should be set to 0.
     */
    size_t data_size;

    /** Private data of specific descriptor storage implementation.
     *
     * Value may be <tt>NULL</tt>, but note that <tt>data_size = 0</tt> in that
     * case.
     */
    void *data;
} EM_descriptor_storage;

typedef struct EM_s
{
    /** Descriptor for <tt>epoll</tt> or <tt>kqueue</tt> event queue.
     *
     * Used only on systems with epoll() system call (e.g. Linux).
     *
     * @default -1
     */
    int queue_fd;

    /** Pipe used for breaking main processing loop in which
     * <tt>epoll_wait()</tt> or <tt>kevent()/kevent64()</tt> is invoked
     *
     * @see event_machine_run()
     */
    int break_loop_pipe[2];

    /** Event descriptor used for registering read end of pipe used for
     * breaking main processing loop.
     *
     * @see #break_loop_pipe
     */
    EM_event_descriptor break_loop_event_descriptor;

    /** Maximum number of events returned by <tt>epoll_wait()</tt> or
     * <tt>kevent()</tt> system call in one batch.
     *
     * This variable limits maximum number of events processed in one batch and
     * also size of <tt>epoll_events</tt> or <tt>kqueue_events</tt> array used
     * for storing <tt>epoll_wait()</tt> or <tt>kevent()</tt> results,
     * respectively.
     *
     * #see #events
     */
    int max_events;

    /** Array used for storing events returned by <tt>epoll_wait()</tt> or
     * <tt>kevent()/kevent64()</tt> function.
     *
     * @default NULL
     *
     * @see #max_events
     * @see #do_free_events
     */
    event_t *events;

    /** Set to true if <tt>events</tt> array was allocated in
     * <tt>event_machine_init()</tt> and false otherwise.
     *
     * @see #events
     * @see #max_events
     */
    bool do_free_events;

    EM_descriptor_storage descriptor_storage;
} EM;

/** Initialize #EM structure.
 *
 * Usage example without user supplied buffer:
 *
 * @code{.c}
 * // Value EM_DEFAULT_MAX_EVENTS is defined in "event-machine.h".
 * struct epoll_event epoll_events[EM_DEFAULT_MAX_EVENTS];
 * EM em = EM_WITH_MAX_EVENTS(EM_DEFAULT_MAX_EVENTS, epoll_events);
 *
 * if_em_failure (event_machine_init(&em))
 * {
 *     // Error handling.
 * }
 * // ...
 * @endcode
 *
 * Usage exampe without user supplied buffer:
 *
 * @code{.c}
 * EM em = EM_WITH_MAX_EVENTS(EM_DEFAULT_MAX_EVENTS, NULL);
 * // Same result can be achieved by:
 * // EM em = EM_DEFAULT;
 *
 * // Function event_machine_init() will allocate array for storing struct
 * // epoll_event. Don't forget to call event_machine_destroy() which frees
 * // that array.
 * if_em_failure (event_machine_init(&em))
 * {
 *     // Error handling.
 * }
 * // ...
 * @endcode
 *
 * @param[in] event_machine
 *   Event machine instance function operates on. Note that some information
 *   are supplied in it. See also macros <tt>EM_STATIC_WITH_MAX_EVENTS</tt>
 *   and <tt>EM_STATIC_DEFAULT</tt>.
 *
 * @return
 *   On success function returns <tt>EM_SUCCESS</tt> and on failure it returns
 *   positive integer from <tt>enum EM_result</tt>.
 */
uint32_t event_machine_init(EM *event_machine);

/** Cleanup any resources associated with <tt>EM</tt> structure.
 *
 * @param[in] event_machine
 *   Event machine instance function operates on.
 *
 * @return
 *   On success function returns <tt>EM_SUCCESS</tt> and on failure it returns
 *   positive integer from <tt>enum EM_result</tt>.
 */
uint32_t event_machine_destroy(EM *event_machine);

/** Start Event Machine main loop.
 *
 * @param[in] event_machine
 *   Event machine instance function operates on.
 *
 * @return
 *   On success function returns <tt>EM_SUCCESS</tt> and on failure it returns
 *   positive integer from <tt>enum EM_result</tt>.
 */
uint32_t event_machine_run(EM *event_machine);

/** Notify Event Machine to break main loop as soon as possible.
 *
 * @param[in] event_machine
 *   Event machine instance function operates on.
 *
 * @return
 *   On success function returns <tt>EM_SUCCESS</tt> and on failure it returns
 *   positive integer from <tt>enum EM_result</tt>.
 */
uint32_t event_machine_terminate(EM *event_machine);

/** Register file descriptor for specified event with its associated data and
 * handler.
 *
 * Do not ever modify event descriptor structure after it was passed to
 * event_machine_add() since it doesn't make any copies and therefore it would
 * cause inconistencies.
 *
 * Usage example:
 *
 * @code{.c}
 * // ...
 * EM_event_descriptor ed =
 * {
 *   .events = EPOLLIN | EPOLLOUT,
 *   .fd = some_fd,
 *   .data = pointer_to_some_private_data,
 *   .handler = pointer_to_event_handling_function
 * };
 *
 * if_em_failure (event_machine_add(em, &ed)
 * {
 *     // Error handling.
 * }
 * // ...
 * @endcode
 *
 * @param[in] event_machine
 *   Event machine instance function operates on. Note that some information
 *   are supplied in it. See also macros <tt>EM_STATIC_WITH_MAX_EVENTS</tt>
 *   and <tt>EM_STATIC_DEFAULT</tt>.
 *
 * @param[in] event_descriptor
 *   Event descriptor caller wants to register.
 *
 * @return
 *   On success function returns <tt>EM_SUCCESS</tt> and on failure it returns
 *   positive integer from <tt>enum EM_result</tt>.
 */
uint32_t event_machine_add(EM *event_machine,
    EM_event_descriptor *event_descriptor);

/** Unregister file descriptor.
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
 *
 * @param[in] event_machine
 *   Event machine instance function operates on.
 *
 * @param[in] fd
 *   File descriptor that caller wants to change registration of. Note that it
 *   has to be the same as value as provided in <tt>event_descriptor</tt>.
 *
 * @param[out] old_event_descriptor
 *   Function returns pointer to old event descriptor so caller can deallocate
 *   it if neccessary. If instead of valid pointer caller supplies
 *   <tt>NULL</tt> then this function won't try to return pointer to old event
 *   descriptor.
 *
 * @return
 *   On success function returns <tt>EM_SUCCESS</tt> and on failure it returns
 *   positive integer from <tt>enum EM_result</tt>.
 */
uint32_t event_machine_delete(EM *event_machine, int fd,
    EM_event_descriptor **old_event_descriptor);

/** Similar as calling event_machine_delete() followed by event_machine_add(),
 * but uses only one epoll_ctl() call.
 *
 * Event descriptor passed to this function should be different from the one
 * used by previous event_machine_add(), because event descriptor may not be
 * changed while it is registered. Function returns event descriptor that was
 * used during event_machine_add() call as does event_machine_delete(). See
 * event_machine_delete() for details.
 *
 * @param[in] event_machine
 *   Event machine instance function operates on.
 *
 * @param[in] fd
 *   File descriptor that caller wants to change registration of. Note that it
 *   has to be the same as value as provided in <tt>event_descriptor</tt>.
 *
 * @param[in] event_descriptor
 *   Event descriptor caller wants to use instead of old one associated with
 *   <tt>fd</tt>.
 *
 * @param[out] old_event_descriptor
 *   Function returns pointer to old event descriptor so caller can deallocate
 *   it if neccessary. If instead of valid pointer caller supplies
 *   <tt>NULL</tt> then this function won't try to return pointer to old event
 *   descriptor.
 *
 * @return
 *   On success function returns <tt>EM_SUCCESS</tt> and on failure it returns
 *   positive integer from <tt>enum EM_result</tt>.
 */
uint32_t event_machine_modify(EM *event_machine, int fd,
    EM_event_descriptor *event_descriptor,
    EM_event_descriptor **old_event_descriptor);

/** Statically set user specified entries of #EM structure.
 *
 * Usage example:
 *
 * @code{.c}
 * struct epoll_event epoll_events[SOME_SIZE];
 * EM em = EM_STATIC_WITH_MAX_EVENTS(SOME_SIZE, epoll_events);
 * // ...
 *
 * if_em_failure (event_machine_init(&em))
 * {
 *     // Error handling.
 * }
 * // ...
 * @endcode
 *
 * @param[in] maxevs
 *   Maximum how many events can event machine process in one iteration.
 *
 * @param[in] evs
 *   Buffer for storing <tt>maxevs</tt> events that occurred on monitored file
 *   descriptors. If <tt>evs = NULL</tt> then event_machine_init() will
 *   allocate space for <tt>maxevs</tt> events. If such memory is allocated
 *   then it is freed by event_machine_destroy().
 */
#define EM_STATIC_WITH_MAX_EVENTS(maxevs, evs)  \
    { .queue_fd = -1                            \
    , .break_loop_pipe = {-1, -1}               \
    , .break_loop_event_descriptor =            \
        { .events = 0                           \
        , .fd = -1                              \
        , .data = NULL                          \
        , .handler = NULL                       \
        }                                       \
    , .max_events = maxevs                      \
    , .events = evs                             \
    , .descriptor_storage =                     \
        { .insert = NULL                        \
        , .remove = NULL                        \
        , .data_size = 0                        \
        , .data = NULL                          \
        }                                       \
    }

/** Statically set user specified entries of #EM structure to their default
 * values.
 *
 * Usage example:
 *
 * @code{.c}
 * EM em = EM_STATIC_DEFAULT;
 * // ...
 *
 * // Function event_machine_init() will allocate array for storing struct
 * // epoll_event. Don't forget to call event_machine_destroy() which frees
 * // that array.
 * if_em_failure (event_machine_init(&em))
 * {
 *     // Error handling.
 * }
 * // ...
 * @endcode
 */
#define EM_STATIC_DEFAULT   \
    EM_STATIC_WITH_MAX_EVENTS(EM_DEFAULT_MAX_EVENTS, NULL)

#ifdef __cplusplus
}
#endif

#endif /* EVENT_MACHINE_H_230071399244842574860511267360184913417 */
