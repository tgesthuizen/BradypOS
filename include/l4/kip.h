#ifndef BRADYPOS_KIP_H
#define BRADYPOS_KIP_H

#include "l4/syscalls.h"
#include <stdint.h>

/**
 * This header describes the data structures used for the KIP page.
 * The KIP page contains information for threads about interacting
 * with the kernel, and its location can be obtained through a
 * syscall.
 * The KIP is mapped into every address space and cannot be unmapped
 * or shared.
 */

struct kern_desc
{
    union
    {
        struct
        {
            uint8_t id;
            uint8_t subid;
        };
        uint32_t raw;
    } kernel_id;
    union
    {
        struct
        {
            uint16_t reserved;
            unsigned year_2000 : 7;
            unsigned month : 4;
            unsigned day : 5;
        };
        uint32_t raw;
    } kernel_gen_date;
    union
    {
        struct
        {
            uint16_t subsubver;
            uint8_t subver;
            uint8_t ver;
        };
        uint32_t raw;
    } kernel_ver;
    char supplier[4];
    char strings[];
};

struct memory_desc
{
    unsigned type : 4;
    unsigned t : 4;
    unsigned reserved : 1;
    unsigned v : 1;
    unsigned low : 22;
    unsigned reserved2 : 10;
    unsigned high : 22;
};

struct proc_desc
{
    uint32_t internal_freq;
    uint32_t external_freq;
};

typedef struct
{
    uint16_t address;
    uint16_t count;
} kip_mem_desc_ptr_t;

typedef union
{
    struct
    {
        uint8_t reserved[2];
        uint8_t subversion;
        uint8_t version;
    };
    uint32_t raw;
} kip_api_version_t;

typedef union
{
    struct
    {
        unsigned ee : 2;
        unsigned ww : 2;
    };
    uint32_t raw;
} kip_api_flags_t;

enum kip_api_endian
{
    kip_api_little_endian,
    kip_api_big_endian
};

enum kip_api_bits
{
    kip_api_32_bits,
    kip_api_64_bits,
};

typedef union
{
    struct
    {
        unsigned s : 4;
        unsigned reserved : 12;
        uint16_t processors;
    };
    uint32_t raw;
} kip_processor_info_t;

typedef union
{
    struct
    {
        unsigned t : 8;
        unsigned system_base : 12;
        unsigned user_base : 12;
    };
    uint32_t raw;
} kip_thread_info_t;

typedef union
{
    struct
    {
        uint16_t schedule_precision;
        uint16_t read_precision;
    };
    uint32_t raw;
} kip_clock_info_t;

typedef union
{
    struct
    {
        unsigned reserved : 10;
        unsigned s : 6;
        unsigned a : 6;
        unsigned m : 10;
    };
    uint32_t raw;
} kip_utcb_info_t;

typedef union
{
    struct
    {
        unsigned reserved : 26;
        unsigned s : 6;
    };
    uint32_t raw;
} kip_area_info_t;

struct kip
{
    uint32_t magic;
    kip_api_version_t api_version;
    kip_api_flags_t api_flags;
    uint32_t kern_desc_ptr;
    uint32_t reserved[17];
    kip_mem_desc_ptr_t memory_info;
    uint32_t reserved2[20];
    uint32_t utcb_info;
    uint32_t kip_area_info;
    uint32_t reserved3[2];
    uint32_t boot_info;
    uint32_t proc_desc_ptr;
    uint32_t clock_info;
    kip_thread_info_t thread_info;
    uint32_t processor_info;

    /**
     * Syscalls are invoked through SVC, instead of these trampolines.
     */
    uint32_t syscalls[12];
};

typedef struct kip L4_kip_t;

static inline void *L4_read_kip_ptr(void *kip, uint32_t kip_ptr)
{
    unsigned char *kip_c = kip;
    return kip_c + kip_ptr;
}

static inline void *L4_kernel_interface(unsigned *api_version,
                                        unsigned *api_flags,
                                        unsigned *kernel_id)
{
    register void *kip asm("r0");
    register unsigned rapi_version asm("r1");
    register unsigned rapi_flags asm("r2");
    register unsigned rkernel_id asm("r3");
    asm volatile("movs r7, %[SYS_KERNEL_INTERFACE]\n\t"
                 "svc #0\n\t"
                 : "=r"(kip), "=r"(rapi_version), "=r"(rapi_flags),
                   "=r"(rkernel_id)
                 : [SYS_KERNEL_INTERFACE] "i"(SYS_KERNEL_INTERFACE)
                 : "r7");
    if (api_version)
        *api_version = rapi_version;
    if (api_flags)
        *api_flags = rapi_flags;
    if (kernel_id)
        *kernel_id = rkernel_id;
    return kip;
}

#endif
