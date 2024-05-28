#ifndef BRADYPOS_KERN_KALARM_H
#define BRADYPOS_KERN_KALARM_H

enum kalarm_type
{
    KALARM_RESCHEDULE,
};

struct kalarm_event
{
    enum kalarm_type type;
    unsigned long when;
    struct kalarm_event *prev;
    struct kalarm_event *next;
};

void kalarm_init();
void register_kalarm_event(struct kalarm_event *event);
void update_kalarm_event(struct kalarm_event *event, unsigned long when);
void remove_kalarm_event(struct kalarm_event *event);
unsigned long get_current_time();

#endif
