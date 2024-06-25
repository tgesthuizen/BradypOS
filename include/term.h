#ifndef BRADYPOS_TERM_H
#define BRADYPOS_TERM_H

enum term_ops
{
    TERM_READ,
    TERM_WRITE,
};

enum term_io_flags
{
    TERM_BLOCK,
};

enum term_read_args
{
    TERM_READ_OP,
    TERM_READ_FLAGS,
    TERM_READ_SIZE,
};

enum term_write_args
{
    TERM_WRITE_OP,
    TERM_WRITE_FLAGS,
    TERM_WRITE_BUF,
};

enum term_answer_ops
{
    TERM_ERROR,
    TERM_READ_RET,
    TERM_WRITE_RET,
};

enum term_read_ret_args
{
    TERM_READ_RET_OP,
    TERM_READ_RET_STR,
};

enum term_write_ret_args
{
    TERM_WRITE_RET_OP,
    TERM_WRITE_RET_SIZE,
};

#endif
