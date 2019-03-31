#pragma once

#include<stddef.h>
#include<mtcommon.h>
typedef atomic_int mutex_t, *mutex_p;

void mutex_acquire(mutex_p pm);
int mutex_try_acquire(mutex_p pm); // Don't use this function for spin lock. Use it to query the state of the mutex.
void mutex_unacquire(mutex_p pm);
