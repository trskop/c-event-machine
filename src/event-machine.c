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

#define _GNU_SOURCE
#include "event-machine.h"
#include "event-machine/result-internal.h"
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <unistd.h>

#define BREAK_LOOP_ED(em)       (em->break_loop_event_descriptor)
#define BREAK_LOOP_PIPE(em)     (em->break_loop_pipe)
#define BREAK_LOOP_READ(em)     (BREAK_LOOP_PIPE(em)[0])
#define BREAK_LOOP_WRITE(em)    (BREAK_LOOP_PIPE(em)[1])

#define STORAGE_INSERT(em)                  (em->descriptor_storage.insert)
#define STORAGE_INSERT_ENTRY(em, fd, ptr)   (STORAGE_INSERT(em)(fd, ptr))
#define STORAGE_REMOVE(em)                  (em->descriptor_storage.remove)
#define STORAGE_REMOVE_ENTRY(em, fd, ptr)   (STORAGE_REMOVE(em)(fd, ptr))


uint32_t event_machine_init(EM *const em)
{
    if_null (em)
    {
        return EM_ERROR_NULL;
    }

    if_invalid_max_events (em->max_events)
    {
        em->epoll_events = NULL;
        em->max_events = EM_DEFAULT_MAX_EVENTS;
    }

    if_null (em->epoll_events)
    {
        em->epoll_events = malloc(sizeof(struct epoll_event) * em->max_events);

        /* Function event_machine_destroy() will call
         * free(em->epoll_events) when this flag is set.
         */
        em->do_free_epoll_events = true;
    }
    else
    {
        em->do_free_epoll_events = false;
    }

    /* We need break_loop_pipe to be non-blocking to prevent writing
     * in to it from blocking.
     *
     * Imagine situation where one thread is running epoll_wait()
     * loop and some other thread calls event_machine_terminate()
     * which writes in to write end of the pipe to notify former that
     * it should terminate. Now if epoll_wait() loop terminates then
     * there is nobody to read data from the pipe. Since pipes have
     * finite buffer size then after filling it up any subsequent
     * write, i.e. event_machine_terminate() call, would be blocked.
     *
     * XXX: To get rid of _GNU_SOURCE declaration we would need to switch to
     *      "pipe() + 2 * fcntl()".
     */
    if_not_zero (pipe2(BREAK_LOOP_PIPE(em), O_CLOEXEC | O_NONBLOCK))
    {
        return EM_ERROR_PIPE;
    }

    /* The break_loop_pipe is used to notify main event handling loop
     * that it should terminate. As a result we need to react to
     * awaiting data for read event on the read end of the pipe. The
     * write end is used by event_machine_terminate() to send the
     * notification.
     */
    BREAK_LOOP_ED(em).events = EPOLLIN;
    BREAK_LOOP_ED(em).fd = BREAK_LOOP_READ(em);

    /* This event is handled internally so there is no need for spplying
     * private data or even event handler.
     */
    BREAK_LOOP_ED(em).data = NULL;
    BREAK_LOOP_ED(em).handler = NULL;

    /* Function epoll_create1() was introduced in Linux kernel 2.6.27 and glibc
     * support was added in version 2.9.
     */
    int epoll_fd = epoll_create1(EPOLL_CLOEXEC);
    if_invalid_fd (epoll_fd)
    {
        return EM_ERROR_EPOLL_CREATE;
    }
    em->epoll_fd = epoll_fd;

    struct epoll_event event =
    {
        .events = BREAK_LOOP_ED(em).events,
        .data.ptr = &(BREAK_LOOP_ED(em))
    };
    if_not_zero (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, BREAK_LOOP_READ(em),
        &event))
    {
        return EM_ERROR_EPOLL_CTL;
    }

    return EM_SUCCESS;
}

uint32_t event_machine_destroy(EM *const em)
{
    if_null (em)
    {
        return EM_ERROR_NULL;
    }

    /* Closing epoll_fd first to ensure that any epoll_ctl() and epoll_wait()
     * on it would fail.
     */
    if_valid_fd (em->epoll_fd)
    {
        if_not_zero (close(em->epoll_fd));
        {
            return EM_ERROR_CLOSE;
        }
        em->epoll_fd = -1;
    }

    /* Freeing memory for storing epoll events after closing epoll file
     * descriptor is important, since there is a possibility that main loop is
     * running while someone called event_machine_destroy() and then we wan't
     * it to fail either on bad faildescriptor (if blocked on epoll_wait()) or
     * on NULL pointer rather then any random memory.
     */
    if (em->do_free_epoll_events)
    {
        void *tmp = em->epoll_events;
        em->epoll_events = NULL;
        free(tmp);
    }

    /* Closing write end of break loop pipe first to make sure that writing in
     * to it would fail.
     */
    if_valid_fd (BREAK_LOOP_WRITE(em))
    {
        if_not_zero (close(BREAK_LOOP_WRITE(em)))
        {
            return EM_ERROR_CLOSE;
        }
        BREAK_LOOP_WRITE(em) = -1;
    }

    if_valid_fd (BREAK_LOOP_READ(em))
    {
        if_not_zero (close(BREAK_LOOP_READ(em)))
        {
            return EM_ERROR_CLOSE;
        }
        BREAK_LOOP_READ(em) = -1;
    }

    return EM_SUCCESS;
}

static inline uint32_t event_machine_run_once(EM *const em,
    const int epoll_fd, struct epoll_event epoll_events[], const int max_events,
    const int break_loop_read_fd, bool *const break_loop)
{
    assert(em != NULL);
    assert(valid_fd(epoll_fd));
    assert(valid_fd(break_loop_read_fd));

    const int num_events = epoll_wait(epoll_fd, epoll_events, max_events, -1);
    if_negative (num_events)
    {
        return EM_ERROR_EPOLL_WAIT;
    }

    for (int i = 0; i < num_events; i++)
    {
        EM_event_descriptor *ed = (EM_event_descriptor *)epoll_events[i].data.ptr;
        if (ed->fd == break_loop_read_fd)
        {
            char ch;

            (*break_loop) = true;
            if_negative (read(break_loop_read_fd, &ch, 1))
            {
                return EM_ERROR_READ;
            }
            /* Case that read() would return 0 doesn't make sense,
             * since that would contradict the fact that we are
             * handling event which is invoked when data are
             * available for reading.
             */
        }
        else
        {
            ed->handler(em, epoll_events[i].events, ed->fd, ed->data);
        }
    }

    return EM_SUCCESS;
}

uint32_t event_machine_run(EM *const em)
{
    if_null (em)
    {
        return EM_ERROR_NULL;
    }
    if_invalid_fd (em->epoll_fd)
    {
        return EM_ERROR_BADFD;
    }
    if_invalid_max_events (em->max_events)
    {
        return EM_ERROR_MAX_EVENTS_TOO_SMALL;
    }
    if_null (em->epoll_events)
    {
        return EM_ERROR_EVENTS_NULL;
    }

    if_negative (fcntl(em->epoll_fd, F_GETFL, 0))
    {
        return EM_ERROR_BADFD;
    }

    for (bool break_loop = false; not(break_loop); )
    {
        uint32_t ret =
            event_machine_run_once(em, em->epoll_fd, em->epoll_events,
                em->max_events, em->break_loop_pipe[0], &break_loop);
        if_em_failure (ret)
        {
            return ret;
        }
    }

    return EM_SUCCESS;
}

uint32_t event_machine_terminate(EM *const em)
{
    if_null (em)
    {
        return EM_ERROR_NULL;
    }

    /* If this file descriptor is invalid it may indicate that either we are
     * trying to call unitialized event machine or if it was already destroyed.
     */
    if_invalid_fd (em->break_loop_pipe[1])
    {
        return EM_ERROR_BADFD;
    }
    if_negative(fcntl(em->break_loop_pipe[1], F_GETFL, 0))
    {
        return EM_ERROR_BADFD;
    }

    char ch = 0;
    if_negative (write(em->break_loop_pipe[1], &ch, 1))
    {
        /* EAGAIN or EWOULDBLOCK indicate that pipe is full and write
         * operation would block. In such case we don't need to write
         * anything and there were no error.
         */
        if (errno != EAGAIN && errno != EWOULDBLOCK)
        {
            return EM_ERROR_WRITE;
        }
    }

    return EM_SUCCESS;
}

uint32_t event_machine_add(EM *const em, EM_event_descriptor *const ed)
{
    if_null (em)
    {
        return EM_ERROR_NULL;
    }
    if_null (ed)
    {
        return EM_ERROR_DESCRIPTOR_NULL;
    }

    /* It doesn't make sense to try to register file descriptor if it's invalid
     * or if epoll_fd is invalid. By checking for it we get more sensible error
     * then if we wen't ahead and called epoll_ctl().
     */
    if (invalid_fd(em->epoll_fd) || invalid_fd(ed->fd))
    {
        errno = EBADF;

        return EM_ERROR_BADFD;
    }
    if (is_negative(fcntl(em->epoll_fd, F_GETFL, 0))
        || is_negative(fcntl(ed->fd, F_GETFL, 0)))
    {
        return EM_ERROR_BADFD;
    }

    struct epoll_event ev = {.events = ed->events, .data.ptr = ed};
    if_not_zero (epoll_ctl(em->epoll_fd, EPOLL_CTL_ADD, ed->fd, &ev))
    {
        if (errno != EEXIST)
        {
            return EM_ERROR_EPOLL_CTL;
        }

        /* It is either trying to add file descriptor that is already there or
         * it encountered kernel bug when using file descriptor that was
         * dup()-ed. Calling epoll_ctl() with EPOLL_CTL_MOD may succeed in such
         * case.
         */
        if_not_zero (epoll_ctl(em->epoll_fd, EPOLL_CTL_MOD, ed->fd, &ev))
        {
            return EM_ERROR_EPOLL_CTL;
        }
    }

    if_not_null (STORAGE_INSERT(em))
    {
        return STORAGE_INSERT_ENTRY(em, ed->fd, ed);
    }

    return EM_SUCCESS;
}

static uint32_t remove_event_descriptor(EM *const em, const int fd,
    EM_event_descriptor **old_ed)
{
    uint32_t ret = EM_SUCCESS;

    assert(em != NULL);
    assert(valid_fd(fd));

    /* Lookup EM_event_descriptor entry associated with fd and return it unless
     * user supplied NULL in which case it is not returned.
     *
     * If em->descriptor_storage.remove is NULL, then either there is no
     * descriptor storage supplied or it doesn't support removing elements.
     * There is perfectly valid (and reasonable) implementation that doesn't
     * support remove. There is however one limitation to such implementation.
     * It may not report EM_ERROR_STORAGE_DUPLICATE_ENTRY when trying to insert
     * new descriptor entry for the same event descriptor, otherwise
     * event_machine_modify() would be unusable.
     */
    if_not_null (STORAGE_REMOVE(em))
    {
        EM_event_descriptor *tmp_ed = NULL;

        ret = STORAGE_REMOVE_ENTRY(em, fd, &tmp_ed);

        /* This way remove function doesn't have to handle NULL pointer and can
         * safely assume that it gets valid storage buffer for event
         * descriptor.
         *
         * Remove function may fail, but it also might return pointer to event
         * descriptor it removed, or tried to remove. We have to handle such
         * case by passing event descriptor pointer before processing its
         * return value.
         */
        if (not_null(tmp_ed) && not_null(old_ed))
        {
            (*old_ed) = tmp_ed;
        }
    }

    return ret;
}

uint32_t event_machine_delete(EM *const em, const int fd,
    EM_event_descriptor **old_ed)
{
    if_null (em)
    {
        return EM_ERROR_NULL;
    }

    /* It doesn't make sense to try to remove file descriptor if it's invalid
     * or if epoll_fd is invalid. By checking for it we get more sensible error
     * then if we wen't ahead and called epoll_ctl().
     */
    if (invalid_fd(em->epoll_fd) || invalid_fd(fd))
    {
        errno = EBADF;

        return EM_ERROR_BADFD;
    }
    if (is_negative(fcntl(em->epoll_fd, F_GETFL, 0))
        || is_negative(fcntl(fd, F_GETFL, 0)))
    {
        return EM_ERROR_BADFD;
    }

    /* Linux kernel versions prior to 2.6.9 require fourth argument to
     * epoll_ctl() to be non-NULL even though it's not used.
     */
    struct epoll_event ev;
    if_not_zero (epoll_ctl(em->epoll_fd, EPOLL_CTL_DEL, fd, &ev))
    {
        return EM_ERROR_EPOLL_CTL;
    }

    return remove_event_descriptor(em, fd, old_ed);
}

uint32_t event_machine_modify(EM *const em, const int fd,
    EM_event_descriptor *const ed, EM_event_descriptor **old_ed)
{
    uint32_t ret = EM_SUCCESS;

    if_null (em)
    {
        return EM_ERROR_NULL;
    }

    /* It doesn't make sense to try to modify file descriptor if it's invalid
     * or if epoll_fd is invalid. By checking for it we get more sensible error
     * then if we wen't ahead and called epoll_ctl().
     */
    if (invalid_fd(em->epoll_fd) || invalid_fd(fd))
    {
        errno = EBADF;

        return EM_ERROR_BADFD;
    }
    if (is_negative(fcntl(em->epoll_fd, F_GETFL, 0))
        || is_negative(fcntl(fd, F_GETFL, 0)))
    {
        return EM_ERROR_BADFD;
    }

    struct epoll_event ev = {.events = ed->events, .data.ptr = ed};
    if_not_zero (epoll_ctl(em->epoll_fd, EPOLL_CTL_MOD, fd, &ev))
    {
        return EM_ERROR_EPOLL_CTL;
    }

    ret_em_failure_of(ret, remove_event_descriptor(em, fd, old_ed));
    if_not_null (STORAGE_INSERT(em))
    {
        return STORAGE_INSERT_ENTRY(em, fd, ed);
    }

    return ret;
}
