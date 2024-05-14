#ifndef BRADYPOS_L4_SPACE_H
#define BRADYPOS_L4_SPACE_H

#include <stdbool.h>

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

inline bool is_nil_fpage(L4_fpage_t fpage) { return fpage.raw == 0; }
inline unsigned floorlog2(unsigned value)
{
    return sizeof(value) - __builtin_clz(value);
}

inline L4_fpage_t L4_fpage(unsigned base_address, int fpage_size)
{
    return (L4_fpage_t){
        .s = floorlog2(fpage_size), .b = base_address, .perm = 0};
}

inline L4_fpage_t L4_fpage_log2(unsigned base_address, int fpage_size_log2)
{
    return (L4_fpage_t){.s = fpage_size_log2, .b = base_address, .perm = 0};
}

inline unsigned L4_address(L4_fpage_t fpage) { return fpage.b; }

inline unsigned L4_size(L4_fpage_t fpage) { return 1 << fpage.s; }
inline unsigned L4_size_log2(L4_fpage_t fpage) { return fpage.s; }
inline unsigned L4_rights(L4_fpage_t fpage) { return fpage.perm; }
inline void L4_set_rights(L4_fpage_t *fpage, unsigned access_rights)
{
    fpage->perm = access_rights;
}
inline L4_fpage_t L4_fpage_add_rights(L4_fpage_t page, unsigned rights)
{
    page.perm |= rights;
    return page;
}
inline L4_fpage_t *L4_fpage_add_rights_to(L4_fpage_t *page, unsigned rights)
{
    page->perm |= rights;
    return page;
}
inline L4_fpage_t L4_fpage_remove_rights(L4_fpage_t page, unsigned rights)
{
    page.perm &= rights;
    return page;
}
inline L4_fpage_t *L4_fpage_remove_right_from(L4_fpage_t *page, unsigned rights)
{
    page->perm &= rights;
    return page;
}



#endif
