#include "event-timer.h"
#include "event-machine/result-internal.h"
#include <assert.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <time.h>
#include <sys/timerfd.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

#define TIMER(data) ((Event_timer*)data)

static void event_timer_internal_timeout(EM *em, uint32_t events, int fd,
    void *data)
{
    uint64_t number_of_timeouts;

    ssize_t readed = read(fd, &number_of_timeouts, sizeof(uint64_t));
    assert(readed == sizeof(uint64_t));

    for(; number_of_timeouts > 0; number_of_timeouts--)
    {
        TIMER(data)->callback(TIMER(data), TIMER(data)->data);
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

    timer->event_machine = event_machine;
    timer->data = data;
    timer->callback = callback;

    int fd = timerfd_create(CLOCK_MONOTONIC, 0);
    if_invalid_fd (fd)
    {
        return EM_ERROR_BADFD;
    }

    timer->event_descriptor.fd = fd;
    timer->event_descriptor.events = EPOLLIN;
    timer->event_descriptor.data = timer;
    timer->event_descriptor.handler = event_timer_internal_timeout;

    return event_machine_add(event_machine, &timer->event_descriptor);
}

void event_timer_start(Event_timer *timer, int32_t msec)
{
    struct itimerspec expiration;

    expiration.it_interval.tv_sec = msec / 1000;
    expiration.it_interval.tv_nsec = (msec % 1000) * 1000;
    expiration.it_value.tv_sec = expiration.it_interval.tv_sec;
    expiration.it_value.tv_nsec = expiration.it_interval.tv_nsec;

    timerfd_settime(timer->event_descriptor.fd, 0, &expiration, NULL);
}

void event_timer_stop(Event_timer *timer)
{
    struct itimerspec expiration;

    memset(&expiration, 0, sizeof(struct itimerspec));
    timerfd_settime(timer->event_descriptor.fd, 0, &expiration, NULL);
}

void event_timer_destroy(Event_timer *timer)
{
    event_machine_delete(timer->event_machine, timer->event_descriptor.fd,
        NULL);
    close(timer->event_descriptor.fd);
}
