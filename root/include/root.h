#ifndef BRADYPOS_ROOT_ROOT_H
#define BRADYPOS_ROOT_ROOT_H

enum root_thread_ops
{
    ROOT_ALLOC_MEM,
    ROOT_FREE_MEM,
};

enum root_thread_alloc_pages_args
{
    root_thread_alloc_mem_op,
    root_thread_alloc_mem_size,
    root_thread_alloc_mem_align,
};

enum root_thread_free_pages_args
{
    root_thread_free_pages_op,
    root_thread_free_pages_ptr,
    root_thread_free_pages_size,
};

enum root_thread_return_codes
{
    ROOT_ALLOC_MEM_RET,
    ROOT_FREE_MEM_RET,
    ROOT_ERROR,
};

#endif
