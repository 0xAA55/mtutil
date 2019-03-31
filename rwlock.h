#pragma once

#include<stdint.h>
#include<stddef.h>
#include<mutex.h>
#include<mtcommon.h>

typedef struct rwlock_struct
{
	mutex_t m;
	atomic_intptr_t r_count;
	atomic_intptr_t w_count;
}rwlock_t, *rwlock_p;

void rwlock_lock_r(rwlock_p l);
void rwlock_lock_w(rwlock_p l);
int rwlock_trylock_r(rwlock_p l);
int rwlock_trylock_w(rwlock_p l);
void rwlock_unlock_r(rwlock_p l);
void rwlock_unlock_w(rwlock_p l);


