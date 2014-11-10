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

#ifndef EVENT_MACHINE_RESULT_H_58077319899644486237516849468267482633
#define EVENT_MACHINE_RESULT_H_58077319899644486237516849468267482633

#ifdef __cplusplus
extern "C" {
#endif

enum EM_result {
    /** Function finished successfully.
     */
    EM_SUCCESS = 0,

    /** Value of <tt>epoll_fd</tt>, or event timer, or any of
     * <tt>break_loop_pipe</tt> file descriptor is invalid.
     */
    EM_ERROR_BADFD = 2,

    /** Value of <tt>max_events</tt> entry of <tt>Event_machine</tt> is
     * invalid.
     */
    EM_ERROR_MAX_EVENTS_TOO_SMALL = 3,

    /** Enum or integer value is out of bounds.
     */
    EM_ERROR_VALUE_OUT_OF_BOUNDS = 4,

    /** Pointer to <tt>Event_machine</tt> type was <tt>NULL</tt>.
     */
    EM_ERROR_NULL = 8,

    /** Value of <tt>epoll_events</tt> entry of <tt>Event_machine</tt> is
     * unexpectedly <tt>NULL</tt>.
     */
    EM_ERROR_EVENTS_NULL = 8 + 1,

    /** Value of Event_descriptor is <tt>NULL</tt>.
     */
    EM_ERROR_DESCRIPTOR_NULL = 8 + 2,

    /** Buffer provided from the outside was <tt>NULL</tt>.
     */
    EM_ERROR_BUFFER_NULL = 8 + 3,

    /** Provided Event_timer pointer is <tt>NULL</tt>.
     */
    EM_ERROR_TIMER_NULL = 8 + 4,

    /** User provided callback function pointer is <tt>NULL</tt>.
     */
    EM_ERROR_CALLBACK_NULL = 8 + 5,

    /** Calling <tt>pipe()</tt> or <tt>pipe2()</tt> failed.
     *
     * See value of <tt>errno</tt> for details.
     */
    EM_ERROR_PIPE = 32 + 1,

    /** Calling <tt>close()</tt> failed.
     *
     * See value of <tt>errno</tt> for details.
     */
    EM_ERROR_CLOSE = 32 + 2,

    /** Calling <tt>read()</tt> failed.
     *
     * See value of <tt>errno</tt> for details.
     */
    EM_ERROR_READ = 32 + 3,

    /** Calling <tt>write()</tt> failed.
     *
     * See value of <tt>errno</tt> for details.
     */
    EM_ERROR_WRITE = 32 + 4,

    /** Calling <tt>epoll_create()</tt> or <tt>epoll_create1()</tt> failed.
     *
     * See value of <tt>errno</tt> for details.
     */
    EM_ERROR_EPOLL_CREATE = 32 + 5,

    /** Calling <tt>epoll_ctl()</tt> failed.
     *
     * See value of <tt>errno</tt> for details.
     */
    EM_ERROR_EPOLL_CTL = 32 + 6,

    /** Calling <tt>epoll_wait()</tt> failed.
     *
     * See value of <tt>errno</tt> for details.
     */
    EM_ERROR_EPOLL_WAIT = 32 + 7,

    /** Calling <tt>timerfd_create()</tt> failed.
     *
     * See value of <tt>errno</tt> for details.
     */
    EM_ERROR_TIMERFD_CREATE = 32 + 8,

    /** Calling <tt>timerfd_settime()</tt> failed.
     *
     * See value of <tt>errno</tt> for details.
     */
    EM_ERROR_TIMERFD_SETTIME = 32 + 9,

    /** Trying to store duplicate event descriptor.
     */
    EM_ERROR_STORAGE_DUPLICATE_ENTRY = 64,

    /** Trying to retrieve non existing event descriptor.
     */
    EM_ERROR_STORAGE_NO_SUCH_ENTRY = 64 + 1
};

#define is_em_success(r)    ((r) == EM_SUCCESS)
#define is_em_failure(r)    (!is_em_success(r))
#define if_em_success(r)    if (is_em_success(r))
#define if_em_failure(r)    if (is_em_failure(r))

#define if_em_success_of(r, function_call)  \
    if_em_success(r = function_call)

#define if_em_failure_of(r, function_call)  \
    if_em_failure(r = function_call)

#define ret_em_failure(r)   \
    if_em_failure(r)        \
    {                       \
        return r;           \
    }

#define ret_em_failure_of(r, function_call) \
    if_em_failure_of(r, function_call)      \
    {                                       \
        return r;                           \
    }

#ifdef __cplusplus
}
#endif

#endif /* EVENT_MACHINE_RESULT_H_58077319899644486237516849468267482633 */
