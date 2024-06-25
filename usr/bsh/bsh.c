#include <l4/ipc.h>
#include <l4/schedule.h>
#include <l4/thread.h>
#include <l4/utcb.h>
#include <service.h>
#include <string.h>
#include <term.h>

enum
{
    IPC_BUFFER_SIZE = 128,
};

register unsigned char *got_location asm("r9");

__attribute__((naked)) void _start()
{
    asm("pop {r0, r1}\n\t"
        "movs r9, r1\n\t"
        "movs r2, #0\n\t"
        "movs lr, r2\n\t"
        "bl main\n\t");
}

extern L4_utcb_t __utcb;
static unsigned char ipc_buffer[IPC_BUFFER_SIZE];
static L4_thread_id term_service;
static L4_thread_id romfs_service;

static L4_thread_id fetch_service(const char *svc)
{
    L4_thread_id root_thread = __utcb.t_pager;
    union
    {
        char c[4];
        unsigned u;
    } v;
    memcpy(&v.c, svc, sizeof(v.c));
    L4_load_mr(
        SERV_GET_OP,
        (L4_msg_tag_t){.u = 1, .t = 0, .flags = 0, .label = SERV_GET}.raw);
    L4_load_mr(SERV_GET_NAME, v.u);
    L4_thread_id from;
    L4_msg_tag_t tag = L4_ipc(root_thread, root_thread,
                              L4_timeouts(L4_never, L4_never), &from);
    if (L4_ipc_failed(tag) || tag.label != SERV_GET_RET || tag.u != 1 ||
        tag.t != 0)
    {
        return L4_NILTHREAD;
    }
    unsigned tid;
    L4_store_mr(SERV_GET_RET_TID, &tid);
    return (L4_thread_id)tid;
}

static L4_thread_id wait_for_service(const char *svc)
{
    L4_thread_id tid = L4_NILTHREAD;
    tid = fetch_service(svc);
    while (L4_is_nil_thread(tid))
    {
        L4_yield();
        tid = fetch_service(svc);
    }
    return tid;
}

int main()
{
    term_service = wait_for_service("term");
    romfs_service = wait_for_service("rom\0");
    while (1)
    {
        L4_yield();
    }
}
