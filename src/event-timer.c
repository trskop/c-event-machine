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


static void internal_timeout_handler(EM *em, uint32_t events, int fd,
    void *data)
{
    uint64_t number_of_timeouts;

    assert(em != NULL);
    assert(valid_fd(fd));

    ssize_t readed = read(fd, &number_of_timeouts, sizeof(uint64_t));
    assert(readed == sizeof(uint64_t));

    for(; number_of_timeouts > 0; number_of_timeouts--)
    {
        Event_timer *timer = CAST_TIMER(data);

        TIMER_CALLBACK(timer)(timer, TIMER_DATA(timer));
    }
}

uint32_t event_timer_create(EM *event_machine, Event_timer *timer,
    Event_timer_handler callback, void *data)
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

    int fd = timerfd_create(CLOCK_MONOTONIC, 0);
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

uint32_t event_timer_start(Event_timer *timer, int32_t msec)
{
    struct itimerspec expiration;

    expiration.it_interval.tv_sec = msec / 1000;
    expiration.it_interval.tv_nsec = (msec % 1000) * 1000;
    expiration.it_value.tv_sec = expiration.it_interval.tv_sec;
    expiration.it_value.tv_nsec = expiration.it_interval.tv_nsec;

    timerfd_settime(TIMER_FD(timer), 0, &expiration, NULL);

    return EM_SUCCESS;
}

uint32_t event_timer_stop(Event_timer *timer)
{
    struct itimerspec expiration;

    memset(&expiration, 0, sizeof(struct itimerspec));
    timerfd_settime(TIMER_FD(timer), 0, &expiration, NULL);

    return EM_SUCCESS;
}

uint32_t event_timer_destroy(Event_timer *timer)
{
    event_machine_delete(TIMER_EM(timer), TIMER_FD(timer), NULL);
    close(TIMER_FD(timer));

    return EM_SUCCESS;
}
