#include"mtcommon.h"
#include"randinst.h"
#include"logprintf.h"
#include"mtatomic.h"

void mt_sleep(unsigned sleeps)
{
#ifdef _WIN32
	Sleep(sleeps);
#else
	usleep(sleeps * 1000);
#endif
}

static void mt_thread_proc(void *param)
{
	thread_info_p t = param;
	backoff_t bo;
	atomic_store(&t->running, 1);
	backoff_init(&bo, 0, 0);
	
	while(atomic_load(&t->running) != 2) backoff_update(&bo);
	
	t->return_value = t->thread_proc(t->param);
	atomic_store(&t->running, 0);
}

#if defined(_WIN32)
static DWORD WINAPI mt_windows_thread_proc(LPVOID param)
{
	mt_thread_proc(param);
	return 0;
}
#endif

#if MT_USE_PTHREAD
static void *mt_pthread_proc(void *param)
{
	mt_thread_proc(param);
	return NULL;
}
#endif

thread_handle_t mt_create_thread(size_t stack_size, int(*thread_proc)(void *param), void *param)
{
	thread_info_p t = NULL;
	backoff_t bo;
#ifdef _WIN32
	DWORD dwThreadId;
#elif MT_USE_PTHREAD
	pthread_attr_t attr;
	int attr_init = 0;
#endif // !MT_USE_PTHREAD

	if(!thread_proc) goto InvalidParamExit;

	backoff_init(&bo, 0, 0);
	
	t = calloc(1, sizeof *t); if(!t) goto GenErrExit;
	t->thread_proc = thread_proc;
	t->param = param;
	
#ifdef _WIN32
	t->h = CreateThread(NULL, stack_size, mt_windows_thread_proc, t, 0, &dwThreadId);
	if(t->h == NULL) goto WinErrExit;
#elif MT_USE_PTHREAD
	if(stack_size)
	{
		if(pthread_attr_init(&attr)) goto GenErrExit;
		attr_init = 1;
		if(pthread_attr_setstacksize(&attr, stack_size)) goto GenErrExit;
	}
	if(pthread_create(&t->p, &attr, mt_pthread_proc, t)) goto GenErrExit;
	pthread_attr_destroy(&attr); attr_init = 0;
#endif // !MT_USE_PTHREAD

	while(atomic_load(&t->running) < 1) backoff_update(&bo);
	atomic_store(&t->running, 2);
	
	return t;
InvalidParamExit:
	logprintf_e("Create new thread failed: Invalid parameters.\n");
	goto FailExit;
#ifdef _WIN32
WinErrExit:
	logprintf_e("Create new thread failed: (%ld) %s\n", GetLastError(), str_GetLastError());
	goto FailExit;
#endif // !_WIN32
GenErrExit:
	logprintf_e("Create new thread failed: (%d) %s\n", errno, strerror(errno));
	goto FailExit;
FailExit:
#if MT_USE_PTHREAD
	if(attr_init) pthread_attr_destroy(&attr);
#endif // !MT_USE_PTHREAD
	mt_free(t);
	return NULL;
}

int mt_get_return_value(thread_handle_t th, int *pret)
{
	thread_info_p t = th;
	if(!t) goto InvalidParamExit;
	
	if(atomic_load(&t->running)) return 0;
	if(pret) *pret = atomic_load(&t->return_value);

	return 1;
InvalidParamExit:
	logprintf_e("Get the return value of a thread failed: Invalid parameters.\n");
	goto FailExit;
FailExit:
	return 0;
}

void mt_free(thread_handle_t th)
{
	thread_info_p t = th;
	backoff_t bo;
	if(!th) return;
	
	backoff_init(&bo, 0, 0);
	
	while(atomic_load(&t->running)) backoff_update(&bo);
#ifdef _WIN32
	if(t->h) CloseHandle(t->h);
#elif MT_USE_PTHREAD
	pthread_join(t->p, NULL);
#endif
	free(t);
}

void backoff_init(backoff_p bo, unsigned max_relaxes, unsigned max_sleeps)
{
	if(!max_relaxes) max_relaxes = MAX_BACKOFF_ITERS;
	if(!max_sleeps) max_sleeps = MAX_BO_SLEEPS;
	memset(bo, 0, sizeof *bo);
	bo->max_relaxes = max_relaxes;
	bo->max_sleeps = max_sleeps;
}

int backoff_update(backoff_p bo)
{
	static int holdrand;
	int r = randi_u32(&holdrand);
	int s = 0;
	
	if(bo->relaxes < bo->max_relaxes)
	{
		if(!bo->relaxes) bo->relaxes = 1;
		else bo->relaxes <<= 1;
	}
	else
	{
		bo->relaxes = bo->max_relaxes;
		s = 1;
		r = randi_u32(&holdrand);
	}

	if(s)
	{
		if(bo->sleeps < bo->max_sleeps)
		{
			if(!bo->sleeps) bo->sleeps = 1;
			else bo->sleeps <<= 1;
		}
		else bo->sleeps = bo->max_sleeps;
		
		mt_sleep(bo->sleeps % r + 1);
	}
	else
	{
		r = bo->relaxes % r + 1;
		while(r--) mt_cpu_relax();
	}
	
	return s;
}


