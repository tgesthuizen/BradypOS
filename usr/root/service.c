#include <errno.h>
#include <l4/ipc.h>
#include <l4/thread.h>
#include <root/service.h>
#include <service.h>
#include <stddef.h>
#include <string.h>

struct service_desc
{
    L4_thread_id thread;
    unsigned name;
};

enum
{
    MAX_SERVICE_COUNT = 16,
};

static int service_count;
static struct service_desc services[MAX_SERVICE_COUNT];

static struct service_desc *find_service_desc(unsigned serv_name)
{
    for (int i = 0; i < service_count; ++i)
    {
        if (services[i].name == serv_name)
            return &services[i];
    }
    return NULL;
}

void make_service_ipc_error(int errno)
{
    L4_load_mr(
        SERV_ERROR_OP,
        (L4_msg_tag_t){.u = 1, .t = 0, .flags = 0, .label = SERV_ERROR}.raw);
    L4_load_mr(SERV_ERROR_ERRNO, errno);
}

L4_thread_id get_service(unsigned name)
{
    struct service_desc *const desc = find_service_desc(name);
    return desc != NULL ? desc->thread : L4_NILTHREAD;
}

void register_service(L4_thread_id tid, unsigned name)
{
    if (service_count >= MAX_SERVICE_COUNT)
        return;
    services[service_count++] =
        (struct service_desc){.thread = tid, .name = name};
}

void register_service_from_name(L4_thread_id tid, char *name)
{
    unsigned uname;
    memcpy(&uname, name, sizeof(uname));
    register_service(tid, uname);
}

void handle_service_get(L4_msg_tag_t tag, L4_thread_id from)
{
    (void)from;
    if (tag.u != 1 || tag.t != 0)
    {
        make_service_ipc_error(EINVAL);
        return;
    }
    unsigned serv_name;
    L4_store_mr(SERV_GET_NAME, &serv_name);
    struct service_desc *const desc = find_service_desc(serv_name);
    if (desc == NULL)
    {
        make_service_ipc_error(ENOENT);
        return;
    }
    L4_load_mr(
        SERV_GET_RET_OP,
        (L4_msg_tag_t){.u = 1, .t = 0, .flags = 0, .label = SERV_GET_RET}.raw);
    L4_load_mr(SERV_GET_RET_TID, desc->thread);
}

void handle_service_register(L4_msg_tag_t tag, L4_thread_id from)
{
    if (tag.u != 1 || tag.t != 0)
    {
        make_service_ipc_error(EINVAL);
        return;
    }
    if (service_count == MAX_SERVICE_COUNT)
    {
        make_service_ipc_error(ENOMEM);
        return;
    }
    if (L4_is_local_id(from))
    {
        from = L4_global_id_of(from);
    }
    unsigned serv_name;
    L4_store_mr(SERV_REGISTER_NAME, &serv_name);
    services[service_count++] =
        (struct service_desc){.name = serv_name, .thread = from};
    L4_load_mr(
        SERV_REGISTER_RET_OP,
        (L4_msg_tag_t){.u = 0, .t = 0, .flags = 0, .label = SERV_REGISTER_RET}
            .raw);
}
