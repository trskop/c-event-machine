/* Copyright (c) 2014, Peter Trško <peter.trsko@gmail.com>
 * 
 * All rights reserved.
 */
#ifndef EVENT_MACHINE_RESULT_H_58077319899644486237516849468267482633
#define EVENT_MACHINE_RESULT_H_58077319899644486237516849468267482633

#ifdef __cplusplus
extern "C" {
#endif

enum EM_result {
    /* Function finished successfully
     */
    EM_SUCCESS = 0,

    /* Value of epoll_fd or any of break_loop_pipe file descriptors  is
     * invalid.
     */
    EM_ERROR_BADFD = 2,

    /* Value of max_events entry of Event_machine is invalid.
     */
    EM_ERROR_MAX_EVENTS_TOO_SMALL = 3,

    /* Enum or integer value is out of bounds. 
     */
    EM_ERROR_VALUE_OUT_OF_BOUNDS = 4,

    /* Pointer to Event_machine type was NULL.
     */
    EM_ERROR_NULL = 8,

    /* Value of epoll_events entry of Event_machine is unexpectedly NULL.
     */
    EM_ERROR_EVENTS_NULL = 8 + 1,

    /* Value of Event_descriptor is NULL.
     */
    EM_ERROR_DESCRIPTOR_NULL = 8 + 2,

    /* Buffer provided from the outside was NULL.
     */
    EM_ERROR_BUFFER_NULL = 8 + 3,

    /* Calling pipe() or pipe2() failed. See ERRNO for details.
     */
    EM_ERROR_PIPE = 32 + 1,

    /* Calling close() failed. See ERRNO for details.
     */
    EM_ERROR_CLOSE = 32 + 2,

    /* Calling close() failed. See ERRNO for details.
     */
    EM_ERROR_READ = 32 + 3,

    /* Calling close() failed. See ERRNO for details.
     */
    EM_ERROR_WRITE = 32 + 4,

    /* Calling epoll_create() or epoll_create1() failed. See ERRNO for details.
     */
    EM_ERROR_EPOLL_CREATE = 32 + 5,

    /* Calling epoll_ctl() failed. See ERRNO for details.
     */
    EM_ERROR_EPOLL_CTL = 32 + 6,

    /* Calling epoll_wait() failed. See ERRNO for details.
     */
    EM_ERROR_EPOLL_WAIT = 32 + 7,

    /* Trying to store duplicate event descriptor.
     */
    EM_ERROR_STORAGE_DUPLICATE_ENTRY = 64,

    /* Trying to retrieve non existing event descriptor.
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
