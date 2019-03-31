#include"mutex.h"
#include"mtcommon.h"
#include"randinst.h"
#include<time.h>

static void mutex_wait_for_acquire(mutex_p pm)
{
	uint32_t iters = 0;
	uint32_t sleeps = 0;
	while(atomic_load(pm) != 0)
	{
		if(iters < MAX_WAIT_ITERS)
		{
			iters ++;
			mt_cpu_relax();
		}
		else
		{
			if(sleeps < MAX_BO_SLEEPS) sleeps ++;
			mt_sleep(sleeps);
		}
	}
}

int mutex_try_acquire(mutex_p pm)
{
	atomic_int xchg = 0;
	if(atomic_load(pm) != 0) return 0;
	if(atomic_compare_exchange_strong(pm, &xchg, 1))
		return 1;
	else
		return 0;
}

static int mutex_rs = 0;

void mutex_acquire(mutex_p pm)
{
	uint32_t max_iters = 1;
	if(!mutex_rs) mutex_rs = (int)time(NULL);
	while(1)
	{
		atomic_int xchg = 0;
		mutex_wait_for_acquire(pm);
		if(atomic_compare_exchange_strong(pm, &xchg, 1))
			break;
		else
		{
			volatile uint32_t i = randi_u32(&mutex_rs) % max_iters;
			max_iters = min(max_iters << 1, MAX_BACKOFF_ITERS);
		
			while(i--) mt_cpu_relax();
		}
	}
}

void mutex_unacquire(mutex_p pm)
{
	atomic_store(pm, 0);
}
