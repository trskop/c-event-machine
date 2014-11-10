#ifndef EVENT_TIMER_H_37644463622302162596821921420243050348
#define EVENT_TIMER_H_37644463622302162596821921420243050348

#include "event-machine.h"

#ifdef __cplusplus
extern "C" {
#endif

struct Event_timer_s;   /* Forward declaration */

typedef void (*Event_timer_handler)(struct Event_timer_s *timer, void *data);

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
* Function will fill Event_timer structure and after that it register 
* event in event machine. Passed event machine must be already 
* initialized and passed Event_timer structure must be allocated.
* 
* @return ::EM_ERROR_NULL
* @return ::EM_ERROR_TIMER_NULL
* @return ::EM_ERROR_CALLBACK_NULL
* @return ::EM_ERROR_BADFD
* @return Errors returned by event_machine_add().
* @return ::EM_SUCCESS
* 
*/
uint32_t event_timer_create(EM *event_machine, Event_timer *timer,
    Event_timer_handler callback, void *data);

/** Start timer with given periode.
*
* @return ::EM_ERROR_TIMER_NULL
* @return ::EM_ERROR_TIMERFD_SETTIME
* @return ::EM_SUCCESS
* 
*/
uint32_t event_timer_start(Event_timer *timer, int32_t msec,
    const bool one_shot);

/** Stop timer.
*
* @return ::EM_ERROR_TIMER_NULL
* @return ::EM_ERROR_TIMERFD_SETTIME
* @return ::EM_SUCCESS
* 
*/
uint32_t event_timer_stop(Event_timer *timer);

/** Uninitialize time and set all structure parts to forbidden values.
*
* @return ::EM_ERROR_TIMER_NULL timer passed to function is NULL.
* @return ::EM_ERROR_CLOSE internal file descriptor is can't be closed from
*         some reason. This sould not happen but we need to be sure.
* @return Errors returned by event_machine_delete().
* @return ::EM_SUCCESS indicale all want sucessfuly
* 
*/
uint32_t event_timer_destroy(Event_timer *timer);

#ifdef __cplusplus
}
#endif

#endif // EVENT_TIMER_H_37644463622302162596821921420243050348
