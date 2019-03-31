#include"rwlock.h"
#include"mtcommon.h"
// #include"projcfg.h"

void rwlock_lock_r(rwlock_p l)
{
	backoff_t bo;
	backoff_init(&bo, 0, 0);
	
	mutex_acquire(&l->m);
	
	while(atomic_load(&l->w_count))
	{
		mutex_unacquire(&l->m);
		backoff_update(&bo);
		mutex_acquire(&l->m);
	}
	
	// I want to use atomic store for cross-processor aware, but I don't want to use atomic_fetch_add which may slower
	atomic_fetch_add(&l->r_count, 1);
	mutex_unacquire(&l->m);
}

void rwlock_lock_w(rwlock_p l)
{
	backoff_t bo;
	backoff_init(&bo, 0, 0);
	
	mutex_acquire(&l->m);
	atomic_fetch_add(&l->w_count, 1);
	
	while(atomic_load(&l->r_count))
	{
		mutex_unacquire(&l->m);
		backoff_update(&bo); // Wait until all readers quit
		mutex_acquire(&l->m);
	}
	
	while(atomic_load(&l->w_count) != 1)
	{
		mutex_unacquire(&l->m);
		backoff_update(&bo); // Wait until only 1 writer here
		mutex_acquire(&l->m);
	}
	
	mutex_unacquire(&l->m);
}

int rwlock_trylock_r(rwlock_p l)
{
	int retval;
	mutex_acquire(&l->m);
	retval = atomic_load(&l->w_count) ? 0 : 1;
	mutex_unacquire(&l->m);
	return retval;
}

int rwlock_trylock_w(rwlock_p l)
{
	int retval;
	mutex_acquire(&l->m);
	retval = (atomic_load(&l->r_count) == 0 && atomic_load(&l->w_count) == 0) ? 1 : 0;
	if(retval) atomic_store(&l->w_count, 1);
	mutex_unacquire(&l->m);
	return retval;
}

void rwlock_unlock_r(rwlock_p l)
{
	mutex_acquire(&l->m);
	atomic_fetch_sub(&l->r_count, 1);
	mutex_unacquire(&l->m);
}

void rwlock_unlock_w(rwlock_p l)
{
	mutex_acquire(&l->m);
	atomic_fetch_sub(&l->w_count, 1);
	mutex_unacquire(&l->m);
}



