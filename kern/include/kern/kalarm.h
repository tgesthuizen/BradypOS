#ifndef BRADYPOS_KERN_KALARM_H
#define BRADYPOS_KERN_KALARM_H

enum kalarm_type
{
    KALARM_RESCHEDULE,
    KALARM_UNPAUSE,
    KALARM_TIMEOUT,
};

struct kalarm_event
{
    unsigned long when;
    enum kalarm_type type;
    unsigned data;
    struct kalarm_event *prev;
    struct kalarm_event *next;
};

void kalarm_init();
void register_kalarm_event(struct kalarm_event *event);
void update_kalarm_event(struct kalarm_event *event, unsigned long when);
void remove_kalarm_event(struct kalarm_event *event);
unsigned long get_current_time();

#endif
