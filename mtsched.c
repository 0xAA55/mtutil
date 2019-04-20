#include"mtsched.h"
#include"randinst.h"
#include"argext.h"
#include<time.h>
#include<logprintf/logprintf.h>

#ifndef MTSCHED_LOG_EVENTS
#define MTSCHED_LOG_EVENTS 1
#endif // !defined(MTSCHED_LOG_EVENTS)

typedef struct mtsched_thread_param_struct
{
	int index;
	mtsched_p s;
}mtsched_thread_param_t, *mtsched_thread_param_p;

static int mtsched_participate_2(mtsched_p s, int thread_index, size_t *queue_index);

static int mtsched_thread_proc(void *param)
{
	mtsched_thread_param_p p = param;
	mtsched_p s = p->s;
	int index = p->index;
	backoff_t bo;
	int initbo = 0;
	int is_idle = 0;
	size_t qi = 0;
	
	atomic_fetch_add_explicit(&s->num_working_threads, 1, memory_order_relaxed);
	
#if MTSCHED_LOG_EVENTS
	logprintf("[mtsched] Thread %d is ready.\n", index);
#endif
	
	for(;;)
	{
		if(atomic_load(&s->quit)) break;
		if(mtsched_participate_2(s, index, &qi))
		{
			initbo = 0;
			is_idle = 0;
		}
		else
		{
			int bo_state;
			if(!initbo)
			{
				backoff_init(&bo, s->max_pause_inst, s->max_wakeup_delay);
				initbo = 1;
			}
			bo_state = backoff_update(&bo);
			if(!bo_state) is_idle = 0;
			else if(!is_idle)
			{
				is_idle = 1;
#if MTSCHED_LOG_EVENTS >= 2
				logprintf("[mtsched] Thread %d entered idle state.\n", index);
#endif
			}
		}
	}
	
	atomic_fetch_sub_explicit(&s->num_working_threads, 1, memory_order_relaxed);
	free(p);
#if MTSCHED_LOG_EVENTS
	logprintf("[mtsched] Thread %d terminated.\n", index);
#endif
	return 0;
}

#if defined(_WIN32)
#include<Windows.h>
int mtsched_get_sys_logic_threads()
{
	SYSTEM_INFO sysinfo;
	GetSystemInfo(&sysinfo);
	return sysinfo.dwNumberOfProcessors;
}
#else
#include<unistd.h>
int mtsched_get_sys_logic_threads()
{
	return sysconf(_SC_NPROCESSORS_ONLN);
}
#endif

mtsched_p mtsched_create(int logic_threads, size_t num_jobs_per_thread, size_t stack_size, void *userdata)
{
	int i;
	int holdrand;
	mtsched_p s = calloc(1, sizeof *s);
	if(!s) goto GenErrExit;

	if(logic_threads < 0 || !num_jobs_per_thread) goto InvalidParamExit;
	if(!logic_threads)
	{
		logic_threads = mtsched_get_sys_logic_threads();
		if (logic_threads <= 0)
#if defined(_WIN32)
			goto WinErrExit;
#else
			goto GenErrExit;
#endif
	}
	if (logic_threads < 1) logic_threads = 1;

	holdrand = (int)time(NULL);
	s->cur_job_id = randi_u32(&holdrand);
	s->num_jobs_per_thread = num_jobs_per_thread;
	
	s->max_jobqueue = logic_threads * num_jobs_per_thread;
	s->jobqueue = calloc(s->max_jobqueue, sizeof s->jobqueue[0]);
	if(!s->jobqueue) goto GenErrExit;
	
	s->max_pause_inst = MAX_BACKOFF_ITERS;
	s->max_wakeup_delay = MAX_BO_SLEEPS;
	
	s->num_thread_handles = logic_threads;
	s->thread_handles = calloc(logic_threads, sizeof s->thread_handles[0]);
	if(!s->thread_handles) goto GenErrExit;
	
	s->stack_size = stack_size;
	s->userdata = userdata;
	
#if MTSCHED_LOG_EVENTS
	logprintf("[mtsched] Initializing scheduler.\n");
#endif
	
	for(i = 0; i < logic_threads; i++)
	{
		mtsched_thread_param_p tp = calloc(1, sizeof *tp);
		if(!tp) goto GenErrExit;
		tp->index = (int)i;
		tp->s = s;
#if MTSCHED_LOG_EVENTS
		logprintf("[mtsched] Creating thread %"PRIsize_t".\n", i);
#endif
		s->thread_handles[i] = mt_create_thread(stack_size, mtsched_thread_proc, tp);
		if(!s->thread_handles[i]){ free(tp); goto FailExit;}
	}
	
	return s;
InvalidParamExit:
	logprintf_e("[mtsched] Create new multithread scheduler failed: Invalid parameters.\n");
	goto FailExit;
#ifdef _WIN32
WinErrExit:
	logprintf_e("[mtsched] Create new multithread scheduler failed: (%ld) %s\n", GetLastError(), str_GetLastError());
	goto FailExit;
#endif // !_WIN32
GenErrExit:
	logprintf_e("[mtsched] Create new multithread scheduler failed: (%d) %s\n", errno, strerror(errno));
	goto FailExit;
FailExit:
	mtsched_free(s);
	return NULL;
}

size_t mtsched_get_numthreads(mtsched_p s)
{
	if(!s) goto InvalidParamExit;
	return s->num_thread_handles;
InvalidParamExit:
	logprintf_e("[mtsched] Get number of threads of a scheduler failed: Invalid parameters.\n");
	goto FailExit;
FailExit:
	return 0;
}

int mtsched_initform(mtsched_p s, mtsched_form_p f, mtsched_proc proc, uint32_t options)
{
	if(!s || !f || !proc) goto InvalidParamExit;
	
	memset(f, 0, sizeof *f);
	f->s = s;
	f->mtsched_func = (void*)proc;
	f->options = options;
	
	return 1;
InvalidParamExit:
	logprintf_e("[mtsched] Initialize a sheduler form failed: Invalid parameters.\n");
	goto FailExit;
FailExit:
	return 0;
}

int mtsched_setparams(mtsched_form_p form, size_t max_args, ...)
{
	va_list ap;
	size_t i;
	
	if(!form) goto InvalidParamExit;
	if(max_args > MTSCHED_MAX_PARAMS) goto TooManyParamsExit;
	if(!max_args) return 1;
	
	form->max_params = max_args;
	
	va_start(ap, max_args);
	for(i = 0; i < max_args; i++) form->params[i] = va_arg(ap, ptrdiff_t);
	va_end(ap);
	return 1;
InvalidParamExit:
	logprintf_e("[mtsched] Set parameters of a sheduler form failed: Invalid parameters.\n");
	goto FailExit;
TooManyParamsExit:
	logprintf_e("[mtsched] Set parameters of a sheduler form failed: Too many parameters (max %u), given %"PRIsize_t" parameters.\n", MTSCHED_MAX_PARAMS, max_args);
	goto FailExit;
FailExit:
	return 0;
}

int mtsched_submit(mtsched_form_p form, uint32_t *out_job_id)
{
	mtsched_p s;
	size_t i, j;
	unsigned loop_times = 0;
	backoff_t bo;
	backoff_init(&bo, 0, 0);
	
	if(!form) goto InvalidParamExit;
	if(!form->s) goto InvalidParamExit;
	if(!out_job_id && (form->options & mtsched_not_monitored) != mtsched_not_monitored) goto InvalidParamExit;

	s = form->s;
	
	for(;;)
	{
		size_t num_finished = 0;
		for(i = 0; i < s->max_jobqueue; i++)
		{
			mtsched_queue_p q = &s->jobqueue[i];
			int found_vacant = 0;
			switch (atomic_load(&q->status))
			{
			case qs_finished:
				num_finished++;
				continue;
			case qs_vacant:
				found_vacant = 1;
				break;
			}

			if(found_vacant)
			{
				uint32_t new_id;
				atomic_int olds = qs_vacant;
				if (atomic_compare_exchange_strong(&q->status, &olds, qs_init))
				{
					rwlock_lock_w(&s->jobqueue_avlbst_lock);
					do
					{
						new_id = s->cur_job_id++;
					} while (avlbst_search(s->jobqueue_avlbst, new_id, NULL) || !new_id);
					avlbst_insert(&s->jobqueue_avlbst, new_id, q);
					rwlock_unlock_w(&s->jobqueue_avlbst_lock);
					q->job_id = new_id;
					q->options = form->options;
					q->max_params = form->max_params;
					for (j = 0; j < form->max_params; j++)
						q->params[j] = form->params[j];
					q->mtsched_func = form->mtsched_func;
					atomic_store(&q->status, qs_pending);
					atomic_fetch_add_explicit(&s->num_pending_jobs, 1, memory_order_relaxed);
					if (out_job_id) *out_job_id = new_id;
#if MTSCHED_LOG_EVENTS >= 3
					logprintf("[mtsched] Scheduled job %08x on %u.\n", new_id, i);
#endif
					return 1;
				}
			}
		}

		if (num_finished == s->max_jobqueue) break;
		
		loop_times ++;
		if(loop_times >= 3)
		{
			if (loop_times >= 6)
			{
#if MTSCHED_LOG_EVENTS >= 2
				logprintf_w("[mtsched] Job queue full.\n");
#endif
				break;
			}
			backoff_update(&bo);
		}
	}
	return 0;
InvalidParamExit:
	logprintf_e("[mtsched] Submit a form to a scheduler failed: Invalid parameters.\n");
	goto FailExit;
FailExit:
	return 0;
}

static int mtsched_proc_pending_job(mtsched_p s, mtsched_queue_p q, int thread_index)
{
	int rv;
	atomic_int olds = qs_pending;
	mtsched_proc fp = (mtsched_proc)q->mtsched_func;
	if (atomic_load(&q->status) != qs_pending) return 0;
	if (!atomic_compare_exchange_strong(&q->status, &olds, qs_processing)) return 0;

	(void)thread_index;

	atomic_fetch_sub_explicit(&s->num_pending_jobs, 1, memory_order_relaxed);
	atomic_fetch_add_explicit(&s->num_processing_jobs, 1, memory_order_relaxed);

#if MTSCHED_LOG_EVENTS >= 3
	if (thread_index >= 0)
		logprintf("[mtsched] Thread %d is participating job %08x at %u.\n", thread_index, q->job_id, thread_index);
	else
		logprintf("[mtsched] Participating job %08x at %u.\n", q->job_id, thread_index);
#endif
	
	if(q->max_params <= 4) rv = fp(TO_ARGS_4(q->params, 0));
	else if(q->max_params <= 8) rv = fp(TO_ARGS_8(q->params, 0));
	else if(q->max_params <= 16) rv = fp(TO_ARGS_16(q->params, 0));
	else if(q->max_params <= 32) rv = fp(TO_ARGS_32(q->params, 0));
	else if(q->max_params <= 64) rv = fp(TO_ARGS_64(q->params, 0));
	else if(q->max_params <= 128) rv = fp(TO_ARGS_128(q->params, 0));
	else rv = fp(TO_ARGS_128(q->params, 0));
	
	if((q->options & mtsched_not_monitored) == mtsched_not_monitored)
	{
		uint32_t id = q->job_id;
		rwlock_lock_w(&s->jobqueue_avlbst_lock);
		avlbst_remove(&s->jobqueue_avlbst, id, NULL);
		rwlock_unlock_w(&s->jobqueue_avlbst_lock);
		atomic_store(&q->status, qs_vacant);
#if MTSCHED_LOG_EVENTS >= 3
		if (thread_index >= 0)
			logprintf("[mtsched] Thread %d had finished job %08x and self receipted.\n", thread_index, q->job_id);
		else
			logprintf("[mtsched] Job %08x had been finished and self receipted.\n", id);
#endif
	}
	else
	{
		q->returned_value = rv;
		atomic_store(&q->status, qs_finished);
#if MTSCHED_LOG_EVENTS >= 3
		if (thread_index >= 0)
			logprintf("[mtsched] Thread %d had finished job %08x.\n", thread_index, q->job_id);
		else
			logprintf("[mtsched] Job %08x had been finished.\n", q->job_id);
#endif
	}
	atomic_fetch_sub_explicit(&s->num_processing_jobs, 1, memory_order_relaxed);
	return 1;
}

int mtsched_participate(mtsched_p s)
{
	return mtsched_participate_2(s, -1, NULL);
}

static int mtsched_participate_2(mtsched_p s, int thread_index, size_t *queue_index)
{
	int did_job = 0;
	size_t i;
	size_t qi = queue_index ? *queue_index : 0;
	if(!s) goto InvalidParamExit;
	
	for(i = 0; i < s->max_jobqueue && !did_job; i++)
	{
		mtsched_queue_p q = &s->jobqueue[(i + qi) % s->max_jobqueue];
		switch (atomic_load(&q->status))
		{
		case qs_pending:
			did_job = mtsched_proc_pending_job(s, q, thread_index);
			if (queue_index) *queue_index = i;
			break;
		}
	}
	
	return did_job;
InvalidParamExit:
	logprintf_e("[mtsched] Participate jobs from a scheduler failed: Invalid parameters.\n");
	goto FailExit;
FailExit:
	return did_job;
}

queue_status_t mtsched_query_job_status(mtsched_p s, uint32_t job_id)
{
	avlbst_p found_node = NULL;
	queue_status_t retval = qs_vacant;
	if(!s) goto InvalidParamExit;
	
	rwlock_lock_r(&s->jobqueue_avlbst_lock);
	if(avlbst_search(s->jobqueue_avlbst, job_id, &found_node))
	{
		mtsched_queue_p q = found_node->userdata;
		retval = atomic_load(&q->status);
	}
	rwlock_unlock_r(&s->jobqueue_avlbst_lock);
	
	return retval;
InvalidParamExit:
	logprintf_e("[mtsched] Query job status failed: Invalid parameters.\n");
	goto FailExit;
FailExit:
	return retval;
}

int mtsched_get_retval(mtsched_p s, uint32_t job_id, int *pret)
{
	int found = 0;
	int retval = 0;
	avlbst_p found_node = NULL;
	
	if(!s) goto InvalidParamExit;
	
	rwlock_lock_r(&s->jobqueue_avlbst_lock);
	if(avlbst_search(s->jobqueue_avlbst, job_id, &found_node))
	{
		mtsched_queue_p q = found_node->userdata;
		found = 1;
		
		if(atomic_load(&q->status) == qs_finished)
		{
			retval = 1;
			if(pret) *pret = q->returned_value;
		}
		else retval = 0;
	}
	rwlock_unlock_r(&s->jobqueue_avlbst_lock);
	
	if(!found) goto JobNotFoundExit;
	return retval;
JobNotFoundExit:
	logprintf_e("[mtsched] Get job %08x retval failed: Job not found.\n", job_id);
	goto FailExit;
InvalidParamExit:
	logprintf_e("[mtsched] Get job %08x retval failed: Invalid parameters.\n", job_id);
	goto FailExit;
FailExit:
	return retval;
}

int mtsched_receipt_job(mtsched_p s, uint32_t job_id)
{
	avlbst_p found_node = NULL;
	if(!s) goto InvalidParamExit;
	
	rwlock_lock_r(&s->jobqueue_avlbst_lock);
	if(avlbst_search(s->jobqueue_avlbst, job_id, &found_node))
	{
		backoff_t bo;
		atomic_int olds = qs_finished;
		mtsched_queue_p q;
		backoff_init(&bo, s->max_pause_inst, s->max_wakeup_delay);
		q = found_node->userdata;
		rwlock_unlock_r(&s->jobqueue_avlbst_lock);
		
		if(atomic_load(&q->status) == qs_pending)
		{
#if MTSCHED_LOG_EVENTS >= 3
			logprintf("[mtsched] Receipting Job %08x but it's still in pending state. Processing the job.\n", job_id);
#endif
			if (!mtsched_proc_pending_job(s, q, -1))
			{
#if MTSCHED_LOG_EVENTS >= 3
				logprintf("[mtsched] Job %08x was occupied by another thread.\n", job_id);
#endif
				backoff_update(&bo);
			}
		}
		while(atomic_load(&q->status) != qs_finished) backoff_update(&bo);
#if MTSCHED_LOG_EVENTS >= 3
		logprintf("[mtsched] Job %08x had been receipted.\n", job_id);
#endif
		rwlock_lock_w(&s->jobqueue_avlbst_lock);
		avlbst_remove(&s->jobqueue_avlbst, job_id, NULL);
		rwlock_unlock_w(&s->jobqueue_avlbst_lock);
		if (!atomic_compare_exchange_strong(&q->status, &olds, qs_vacant)) goto UnexpectedErrorExit;
	}
	else rwlock_unlock_r(&s->jobqueue_avlbst_lock);
	
	if(!found_node) goto NotFoundExit;
	return 1;
NotFoundExit:
	logprintf_e("[mtsched] Receipt job failed: Job not found: %u.\n", job_id);
	goto FailExit;
InvalidParamExit:
	logprintf_e("[mtsched] Receipt job failed: Invalid parameters.\n");
	goto FailExit;
UnexpectedErrorExit:
	logprintf_e("[mtsched] Receipt job failed: Unexpected error.\n");
	goto FailExit;
FailExit:
	return 0;
}

void mtsched_free(mtsched_p s)
{
	if(s)
	{
		size_t i;
		backoff_t bo;
		backoff_init(&bo, 0, 0);
		
		atomic_store(&s->quit, 1);
#if MTSCHED_LOG_EVENTS
		logprintf("[mtsched] Quit signal sent.\n");
#endif
		while(atomic_load(&s->num_working_threads)) backoff_update(&bo);
		
		if(s->thread_handles)
		{
			for(i = 0; i < s->num_thread_handles; i ++)
				mt_free(s->thread_handles[i]);
			free(s->thread_handles);
		}
		avlbst_free(&s->jobqueue_avlbst, NULL);
		free(s->jobqueue);
		free(s);
	}
}
