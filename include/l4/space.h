#ifndef BRADYPOS_L4_SPACE_H
#define BRADYPOS_L4_SPACE_H

#include "l4/thread.h"
#include <l4/ipc.h>
#include <l4/syscalls.h>
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

inline void L4_unmap(unsigned control)
{
    register unsigned rcontrol asm("r0") = control;
    asm volatile("movs r7, %[SYS_UNMAP]\n\t"
                 "svc #0\n\t" ::[SYS_UNMAP] "i"(SYS_UNMAP),
                 "r"(rcontrol));
}

inline L4_fpage_t L4_unmap_fpage(L4_fpage_t f)
{
    L4_load_mr(0, f.raw);
    L4_unmap(0);
    L4_store_mr(0, &f.raw);
    return f;
}

inline void L4_unmap_fpages(unsigned n, L4_fpage_t *fpages)
{
    L4_load_mrs(0, n, (unsigned *)fpages);
    L4_unmap(n - 1);
    L4_store_mrs(0, n, (unsigned *)fpages);
}

inline L4_fpage_t L4_flush_fpage(L4_fpage_t f)
{
    L4_load_mr(0, f.raw);
    L4_unmap(64);
    L4_store_mr(0, &f.raw);
    return f;
}

inline void L4_flush_fpages(unsigned count, L4_fpage_t *pages)
{
    L4_load_mrs(0, count, (unsigned *)pages);
    L4_unmap(64 + count - 1);
    L4_store_mrs(0, count, (unsigned *)pages);
}

inline L4_fpage_t L4_get_status(L4_fpage_t fpage)
{
    L4_load_mr(0, L4_fpage_remove_rights(fpage, L4_fully_accessible).raw);
    L4_unmap(0);
    L4_store_mr(0, &fpage.raw);
    return fpage;
}

inline bool L4_was_referenced(L4_fpage_t fpage)
{
    return (fpage.perm & L4_readable) != 0;
}

inline bool L4_was_written(L4_fpage_t fpage)
{
    return (fpage.perm & L4_writable) != 0;
}

inline bool L4_was_executed(L4_fpage_t fpage)
{
    return (fpage.perm & L4_executable) != 0;
}

// 4.3 Space Control

inline unsigned L4_space_control(L4_thread_id space_specifier, unsigned control,
                                 L4_fpage_t kip_area, L4_fpage_t utcb_area,
                                 L4_thread_id redirector, unsigned *old_control)
{
    unsigned result;
    register unsigned rspace_specifier asm("r0") = space_specifier;
    register unsigned rcontrol asm("r1") = control;
    register unsigned rkip_area asm("r2") = kip_area.raw;
    register unsigned rutcb_area asm("r3") = utcb_area.raw;
    register unsigned rredirector asm("r4") = redirector;
    asm volatile("movs r7, %[SYS_SPACE_CONTROL]\n\t"
                 "svc #0\n\t"
                 : "=r"(result), "=r"(control)
                 : [SYS_SPACE_CONTROL] "i"(SYS_SPACE_CONTROL),
                   "0"(rspace_specifier), "1"(rcontrol), "r"(rkip_area),
                   "r"(rutcb_area), "r"(rredirector));
    *old_control = control;
    return result;
}

#endif
