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

uint8_t event_timer_creat(EM *event_machine, Event_timer* timer,
    Event_timer_handler callback, void *data);

void event_timer_start(Event_timer *timer, int32_t msec);

void event_timer_stop(Event_timer *timer);

void event_timer_destroy(Event_timer *timer);

#ifdef __cplusplus
}
#endif

#endif // EVENT_TIMER_H_37644463622302162596821921420243050348
