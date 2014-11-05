#ifndef EVENT_TIMER_H_37644463622302162596821921420243050348
#define EVENT_TIMER_H_37644463622302162596821921420243050348

#include "event-machine.h"

#ifdef __cplusplus
extern "C" {
#endif

struct Event_timer_s;

typedef void (*Event_timer_handler)(struct Event_timer_s *timer, void *data);

typedef struct Event_timer_s
{
    EM_event_descriptor event_descriptor;
    EM *event_machine;
    void *data;
    Event_timer_handler callback;

}Event_timer;


uint8_t event_timer_creat(EM *event_machine, Event_timer* timer,
    Event_timer_handler callback, void *data);

void event_timer_start(Event_timer *timer, int32_t msec);

void event_timer_stop();

void event_timer_destroy(Event_timer *timer);

#ifdef __cplusplus
}
#endif

#endif // EVENT_TIMER_H_37644463622302162596821921420243050348
