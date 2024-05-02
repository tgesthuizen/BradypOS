#ifndef BRADYPOS_KERN_SOFTIRQ_H
#define BRADYPOS_KERN_SOFTIRQ_H

enum softirq_type_t
{
    SIRQ_SVC_CALL,

    SIRQ_LAST
};

void softirq_schedule(enum softirq_type_t type);
void softirq_execute();

#endif
