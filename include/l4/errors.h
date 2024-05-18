#ifndef BRADYPOS_L4_ERRORS_H
#define BRADYPOS_L4_ERRORS_H

enum L4_error_code
{
    L4_error_no_privilege = 1,
    L4_error_invalid_thread = 2,
    L4_error_invalid_space = 3,
    L4_error_invalid_scheduler = 4,
    L4_error_invalid_parameter = 5,
    L4_error_utcb_area = 6,
    L4_error_kip_area = 7,
    L4_error_no_memory = 8,
};

typedef enum L4_error_code L4_error_code_t;

#endif
