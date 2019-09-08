#include "event_threads.h"
#include "psx.h"

#include <lv2/process.h>
#include <sys/file.h>
#include <ppu-lv2.h>
#include <sys/stat.h>
#include <lv2/sysfs.h>

#include <sysutil/disc.h>

#include <sys/thread.h>
#include <sys/event_queue.h>

static sys_ppu_thread_t thread_id;

static sys_event_queue_t evQ_sd;
static sys_event_port_t portId;

static volatile int event_thread_working = 0;

static sys_event_queue_attr_t evQAttr_sd = { SYS_EVENT_QUEUE_FIFO, SYS_EVENT_QUEUE_PPU, "EvTh" };

static void Event_thread(void *a)
{
    sys_event_t event_sd;

    void (*func)(void * priv) = NULL;

    while(true)
    {
        if(sysEventQueueReceive(evQ_sd, &event_sd, 0) < 0) break;
        if(event_sd.data_1 == 0x666) break;


        if(event_sd.data_1 == 0x555)
        {
            event_thread_working = 1;
            func = (void *) event_sd.data_2;
            if(func) func((void *) event_sd.data_3);
            event_thread_working = 0;
        }
    }

    sysThreadExit(0);

}

static int init = 0;

void event_threads_init()
{
    if(init) return;

    init = 1;

#ifdef PSDEBUG
    int ret=
#endif
    sysEventQueueCreate(&evQ_sd, &evQAttr_sd, 0xAAAA4242, 16);

#ifdef PSDEBUG
    ret =
#endif
    sysEventPortCreate(&portId, 1, 0xAAAA4242);

#ifdef PSDEBUG
    ret =
#endif
    sysEventPortConnectLocal(portId, evQ_sd);

    sysThreadCreate(&thread_id, Event_thread, NULL, 992, 0x100000/4, THREAD_JOINABLE, "Event_thread");
}

void event_threads_finish()
{
    u64 retval;

    if(!init) return;

    event_thread_send(0x666, 0, 0);
    sysThreadJoin(thread_id, &retval);

    sysEventPortDestroy(portId);
    sysEventQueueDestroy(evQ_sd, 0);

    init = 0;
}


int event_thread_send(u64 data0, u64 data1, u64 data2)
{
    return sysEventPortSend(portId, data0, data1, data2);
}

void wait_event_thread()
{
    while(event_thread_working) usleep(1000);
}
