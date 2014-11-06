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
        return EM_ERROR_BADFD;
    }

    TIMER_ED(timer).fd = fd;
    TIMER_ED(timer).events = EPOLLIN;
    TIMER_ED(timer).data = timer;
    TIMER_ED(timer).handler = internal_timeout_handler;

    return event_machine_add(event_machine, &TIMER_ED(timer));
}

uint32_t event_timer_start(Event_timer *const timer, const int32_t msec)
{
    if_null (timer)
    {
        return EM_ERROR_TIMER_NULL;
    }

    struct itimerspec expiration =
    {
        .it_interval =
        {
            .tv_sec = msec / 1000,
            .tv_nsec = (msec % 1000) * 1000
        },
        .it_value =
        {
            .tv_sec = expiration.it_interval.tv_sec,
            .tv_nsec = expiration.it_interval.tv_nsec
        }
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

    /* Make sure that all information stored in Event_timer structure are lost.
     * Since most of the entries are pointers this is the simplest way, but it
     * is good practice to set file descriptors to -1,  because its an invalid
     * value.
     */
    memset(timer, 0, sizeof(Event_timer));
    TIMER_FD(timer) = -1;
    TIMER_ED(timer).fd = -1;

    return EM_SUCCESS;
}
