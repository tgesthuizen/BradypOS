#ifndef BRADYPOS_KERN_KALARM_H
#define BRADYPOS_KERN_KALARM_H

void kalarm_init();
void register_kalarm_event(unsigned long when, void (*what)());
unsigned long get_current_time();

#endif
