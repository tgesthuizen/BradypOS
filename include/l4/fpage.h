#ifndef L4_FPAGE_H
#define L4_FPAGE_H

// L4 specification 4.1

union L4_fpage
{
    struct
    {
        unsigned perm : 4;
        unsigned s : 6;
        unsigned b : 22;
    };
    unsigned raw;
};

typedef union L4_fpage L4_fpage_t;

enum L4_fpage_permissions
{
    L4_readable = 0b0100,
    L4_writable = 0b0010,
    L4_executable = 0b0001,
    L4_fully_accessible = 0b0111,
    L4_read_exec_only = 0b0101,
    L4_no_access = 0b0000,
};

#define L4_complete_address_space ((L4_fpage_t){.perm = 0, .s = 1, .b = 0})
#define L4_nilpage ((L4_fpage_t){.raw = 0})

#endif
