#ifndef BRADYPOS_SERVICE_H
#define BRADYPOS_SERVICE_H

enum service_ops
{
    SERV_GET,
    SERV_REGISTER,
};

enum service_get_args
{
    SERV_GET_OP,
    SERV_GET_NAME,
};

enum service_register_args
{
    SERV_REGISTER_OP,
    SERV_REGISTER_NAME,
};

enum service_ret_ops
{
    SERV_ERROR,
    SERV_GET_RET,
    SERV_REGISTER_RET,
};

enum service_error_args
{
    SERV_ERROR_OP,
    SERV_ERROR_ERRNO,
};

enum service_get_ret_args
{
    SERV_GET_RET_OP,
    SERV_GET_RET_TID,
};

enum service_register_ret_args
{
    SERV_REGISTER_RET_OP,
};

#endif
