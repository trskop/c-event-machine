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

/** @file event-timer.h
 * Interface for simple timers based on event-machine approach.
 * Usage example can be found here: @link example/timer.c @endlink
 *
 * @author Peter Trško
 * @author Jan Šipr
 * @date 2014
 * @copyright BSD3
 * 
 * @example example/timer.c
 *   Timer example that is based on @link example/stdin-stdout.c @endlink
 *   example to make it interruptible by closing standard input
 *   (<tt>CTRL-D</tt> on POSIX-like platforms). For details see mentioned
 *   @link example/stdin-stdout.c @endlink.
 */

#ifndef EVENT_TIMER_H_37644463622302162596821921420243050348
#define EVENT_TIMER_H_37644463622302162596821921420243050348

#include "event-machine.h"

#ifdef __cplusplus
extern "C" {
#endif

struct Event_timer_s;   /* Forward declaration */

/** Type of callbacks triggered by timer expiration.
 *
 * @param[in] timer
 *   Event_timer structure of timer that expired and therefore this callback
 *   was invoked.
 *
 * @param[in] data
 *   Pointer to private timer data that were passed to it in
 *   event_timer_create() call. It may be <tt>NULL</tt>.
 */
typedef void (*Event_timer_handler)(struct Event_timer_s *timer, void *data);

/** Structure that describes event timer.
 *
 * This structure behaves as timer handle and it has to be first initialized
 * using event_timer_create(). Note that function event_timer_create() doesn't
 * do allocation and therefore valid pointer to Event_timer structure has to be
 * presented. After that instance of this structure is used as an argument to
 * every timer related function call.
 */
typedef struct Event_timer_s
{
    /** Event descriptor used to register timer instance in provided
     * event machine instance.
     */
    EM_event_descriptor event_descriptor;

    /** Event machine in which this timer instance was/will be
     * registered.
     */
    EM *event_machine;

    /** Private data associated with this instance of timer.
     *
     * It's value is passed down to <tt>callback</tt> and it may be
     * <tt>NULL</tt>.
     */
    void *data;

    /** Callback which is invoked when timer fires.
     */
    Event_timer_handler callback;
} Event_timer;

/** Create and register event timer in event machine.
 *
 * Function registers timer in event_machine, therefore argument event_machine
 * has to be valid (already initialized) event machine.
 *
 * @param[in] event_machine
 *   Initialized Event_machine structure. If <tt>event_machine = NULL</tt> then
 *   this function fails with #EM_ERROR_NULL.
 *
 * @param[in] timer
 *   Already allocated buffer where Event_timer structure will be stored and it
 *   is later used by event_timer_start() and event_timer_stop() functions.
 *   Before deallocating timer function event_timer_destroy() has to be called.
 *   If <tt>timer = NULL</tt> then this function will return
 *   #EM_ERROR_TIMER_NULL.
 *
 * @param[in] callback
 *   Callback function that is invoked when timer expires. This value may not
 *   be <tt>NULL</tt> otherwise this function would return
 *   #EM_ERROR_CALLBACK_NULL.
 *
 * @param[in] data
 *   Pointer to timer private state/data. It is passed to each invocation of
 *   callback function and its value may be <tt>NULL</tt>.
 *
 * @return
 *   Returns #EM_ERROR_NULL if pointer to event machine (EM structure) is
 *   <tt>NULL</tt>.
 *
 * @return
 *   Returns #EM_ERROR_TIMER_NULL when timer passed to function is
 *   <tt>NULL</tt>.
 *
 * @return
 *   Returns #EM_ERROR_CALLBACK_NULL when argument with callback function
 *   pointer is <tt>NULL</tt>.
 *
 * @return
 *   Returns #EM_ERROR_TIMERFD_CREATE if timerfd_create() fails. Read value of
 *   <tt>errno</tt> for details.
 *
 * @return
 *   Errors returned by event_machine_add().
 *
 * @return
 *   On success function returns #EM_SUCCESS.
 */
uint32_t event_timer_create(EM *event_machine, Event_timer *timer,
    Event_timer_handler callback, void *data);

/** Start timer with given periode.
 *
 * @param[in] timer
 *   Event timer to start. This value has to be initialized by
 *   event_timer_create(). If <tt>timer = NULL</tt> then this function will
 *   return #EM_ERROR_TIMER_NULL.
 *
 * @param[in] msec
 *   Expiration timeof this timer in miliseconds. If value of
 *   <tt>is_one_shot = true</tt>, then timer is fired only once after this
 *   specified period of time, but if <tt>is_one_shot = false</tt> then this
 *   timer is fired periodically, each time after this specified time value.
 *
 * @param[in] is_one_shot
 *   Boolean that indicates if timer should be fired only once or periodically.
 *
 * @return
 *   Returns #EM_ERROR_TIMER_NULL when timer passed to function is
 *   <tt>NULL</tt>.
 *
 * @return
 *   Returns #EM_ERROR_TIMERFD_SETTIME if timerfd_settime() fails. Read value
 *   of <tt>errno</tt> for details.
 *
 * @return
 *   On success function returns #EM_SUCCESS.
 */
uint32_t event_timer_start(Event_timer *timer, int32_t msec,
    const bool is_one_shot);

/** Stop timer.
 *
 * @param[in] timer
 *   Event timer to stop. This value has to be initialized by
 *   event_timer_create(). If <tt>timer = NULL</tt> then this function will
 *   return #EM_ERROR_TIMER_NULL.
 *
 * @return
 *   Returns #EM_ERROR_TIMER_NULL when timer passed to function is
 *   <tt>NULL</tt>.
 *
 * @return
 *   Returns #EM_ERROR_TIMERFD_SETTIME if timerfd_settime() fails. Read value
 *   of <tt>errno</tt> for details.
 *
 * @return
 *   On success function returns #EM_SUCCESS.
 */
uint32_t event_timer_stop(Event_timer *timer);

/** Uninitialize time and set all structure parts to forbidden values.
 *
 * @param[in] timer
 *   Event timer to destroy. This value has to be initialized by
 *   event_timer_create(). If <tt>timer = NULL</tt> then this function will
 *   return #EM_ERROR_TIMER_NULL.
 *
 * @return
 *   Returns #EM_ERROR_TIMER_NULL when timer passed to function is
 *   <tt>NULL</tt>.
 *
 * @return
 *   Returns #EM_ERROR_CLOSE if <tt>close()</tt> call on timerfd file
 *   descriptor fails. Read value of <tt>errno</tt> for details.
 *
 * @return
 *   Errors returned by event_machine_delete().
 *
 * @return
 *   On success function returns #EM_SUCCESS.
 */
uint32_t event_timer_destroy(Event_timer *timer);

#ifdef __cplusplus
}
#endif

#endif // EVENT_TIMER_H_37644463622302162596821921420243050348
