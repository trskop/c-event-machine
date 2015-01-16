/* Copyright (c) 2014, 2015, Peter Trško <peter.trsko@gmail.com>
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

#if !defined(_POSIX_C_SOURCE) || _POSIX_C_SOURCE < 199901L
#define _POSIX_C_SOURCE 199901L
#endif

#include "event-timer.h"
#include "event-machine/result-internal.h"
#include <assert.h>
#include <errno.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/timerfd.h>
#include <unistd.h>

#define CAST_TIMER(data)        ((Event_timer*)data)

/* Accessor macros for various Event_timer fields. Please use these in case
 * that its internal structure changes.
 */
#define TIMER_FD(timer)         (timer->event_descriptor.fd)
#define TIMER_EM(timer)         (timer->event_machine)
#define TIMER_ED(timer)         (timer->event_descriptor)
#define TIMER_DATA(timer)       (timer->data)
#define TIMER_CALLBACK(timer)   (timer->callback)


static void internal_timeout_handler(EM *const em, const uint32_t events,
    const int fd, void *const data)
{
    uint64_t number_of_timeouts;

    assert(em != NULL);
    assert(valid_fd(fd));

    ssize_t readed = read(fd, &number_of_timeouts, sizeof(uint64_t));
    if (readed != sizeof(uint64_t) && errno == EAGAIN)
    {
        /* Since timer file descriptor is registered as level triggered, then
         * we can safely retry later.
         */
        return;
    }
    assert(readed == sizeof(uint64_t));

    for(; number_of_timeouts > 0; number_of_timeouts--)
    {
        Event_timer *timer = CAST_TIMER(data);

        TIMER_CALLBACK(timer)(timer, TIMER_DATA(timer));
    }
}

static void shred(Event_timer *const timer)
{
    /* Make sure that all information stored in Event_timer structure are lost.
     * Since most of the entries are pointers this is the simplest way, but it
     * is good practice to set file descriptors to -1,  because its an invalid
     * value.
     */
    memset(timer, 0, sizeof(Event_timer));
    TIMER_FD(timer) = -1;
    TIMER_ED(timer).fd = -1;
}

uint32_t event_timer_create(EM *const event_machine, Event_timer *const timer,
    const Event_timer_handler callback, void *const data)
{
    if_null (event_machine)
    {
        return EM_ERROR_NULL;
    }
    if_null (timer)
    {
        return EM_ERROR_TIMER_NULL;
    }
    if_null (callback)
    {
        return EM_ERROR_CALLBACK_NULL;
    }

    TIMER_EM(timer) = event_machine;
    TIMER_DATA(timer) = data;
    TIMER_CALLBACK(timer) = callback;

    int fd = timerfd_create(CLOCK_MONOTONIC, TFD_CLOEXEC | TFD_NONBLOCK);
    if_invalid_fd (fd)
    {
        return EM_ERROR_TIMERFD_CREATE;
    }

    TIMER_ED(timer).fd = fd;
    TIMER_ED(timer).events = EPOLLIN;
    TIMER_ED(timer).data = timer;
    TIMER_ED(timer).handler = internal_timeout_handler;

    uint32_t ret = event_machine_add(event_machine, &TIMER_ED(timer));
    if_em_failure (ret)
    {
        int saved_errno = errno;

        /* Already checked that event_machine != NULL.
         */
        assert(ret != EM_ERROR_NULL);

        /* Passed Event_descriptor is part of Event_timer structure
         * and it was checked that timer != NULL
         */
        assert(ret != EM_ERROR_DESCRIPTOR_NULL);

        /* If close() or something in shred() fails then its errno is ignored.
         * At this point error returned by event_machine_add() has more
         * priority.
         *
         * If for some reason event_machine_add() returned error and fd was
         * registered with epoll, then closing it is correct thing to do. Epoll
         * handles closed file descriptors by removing them.
         */
        close(fd);
        shred(timer);
        errno = saved_errno;
    }

    return ret;
}

uint32_t event_timer_start(Event_timer *const timer, const int32_t msec,
    const bool is_one_shot)
{
    if_null (timer)
    {
        return EM_ERROR_TIMER_NULL;
    }

    struct timespec expiration_time =
    {
        .tv_sec = msec / 1000,
        .tv_nsec = (msec % 1000) * 1000
    };
    struct timespec expiration_time_zero = {0, 0};
    struct itimerspec expiration =
    {
        .it_interval = is_one_shot ? expiration_time_zero : expiration_time,
        .it_value = expiration_time
    };

    if_negative (timerfd_settime(TIMER_FD(timer), 0, &expiration, NULL))
    {
        return EM_ERROR_TIMERFD_SETTIME;
    }

    return EM_SUCCESS;
}

uint32_t event_timer_stop(Event_timer *const timer)
{
    struct itimerspec expiration = {{0, 0}, {0, 0}};

    if_null (timer)
    {
        return EM_ERROR_TIMER_NULL;
    }

    if_negative (timerfd_settime(TIMER_FD(timer), 0, &expiration, NULL))
    {
        return EM_ERROR_TIMERFD_SETTIME;
    }

    return EM_SUCCESS;
}

uint32_t event_timer_destroy(Event_timer *const timer)
{
    if_null (timer)
    {
        return EM_ERROR_TIMER_NULL;
    }

    /* Last argument to event_machine_delete() is NULL since Event_descriptor
     * is part of Event_timer data structure it doesn't make sense to request
     * pointer to it.
     */
    int ret = event_machine_delete(TIMER_EM(timer), TIMER_FD(timer), NULL);
    if_em_failure (ret)
    {
        int saved_errno = errno;

        /* Already checked that event_machine != NULL.
         */
        assert(ret != EM_ERROR_NULL);

        /* Passed Event_descriptor is part of Event_timer structure
         * and it was checked that timer != NULL
         */
        assert(ret != EM_ERROR_DESCRIPTOR_NULL);

        /* If close() fails then its errno is ignored. At this point error
         * returned by event_machine_delete() has more priority.
         */
        close(TIMER_FD(timer));
        errno = saved_errno;

        return ret;
    }

    if_negative (close(TIMER_FD(timer)))
    {
        return EM_ERROR_CLOSE;
    }

    shred(timer);

    return EM_SUCCESS;
}
