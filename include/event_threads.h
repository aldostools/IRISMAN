#ifndef EVENT_THREADS_H
#define EVENT_THREADS_H

#include "utils.h"

void event_threads_init();

void event_threads_finish();

int event_thread_send(u64 data0, u64 data1, u64 data2);

void wait_event_thread();

#endif

