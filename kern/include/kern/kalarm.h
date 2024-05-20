#ifndef BRADYPOS_KERN_KALARM_H
#define BRADYPOS_KERN_KALARM_H

enum kalarm_type
{
    KALARM_RESCHEDULE,
};

void kalarm_init();
void register_kalarm_event(enum kalarm_type, unsigned long when);
void update_kalarm_event(enum kalarm_type type, unsigned long when);
unsigned long get_current_time();

#endif
