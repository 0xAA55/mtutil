#include"mtschedman.h"
#include<stdlib.h>
#include<logprintf/logprintf.h>

#ifndef MTSCHEDMAN_LOG_EVENTS
#define MTSCHEDMAN_LOG_EVENTS 1
#endif // !defined(MTSCHEDMAN_LOG_EVENTS)

mtsched_manager_p mtschedman_create(mtsched_p s)
{
	mtsched_manager_p m = NULL;
	
	if(!s) goto InvalidParamExit;
	
	m = calloc(1, sizeof *m);
	if(!m) goto GenErrExit;
	
	m->s_ref = s;
	m->max_joblist = s->max_jobqueue;
	m->joblist = calloc(m->max_joblist, sizeof m->joblist[0]);
	if(!m->joblist) goto FailExit;

#if MTSCHEDMAN_LOG_EVENTS >= 2
	logprintf("[mtschedman] Instance created.\n");
#endif
	
	return m;
InvalidParamExit:
	logprintf_e("[mtschedman] Create instance failed: Invalid parameters.\n");
	goto FailExit;
GenErrExit:
	logprintf_e("[mtschedman] Create instance failed: (%d) %s\n", errno, strerror(errno));
	goto FailExit;
FailExit:
	mtschedman_free(m);
	return NULL;
}

static int mtschedman_receipt_finished_jobs(mtsched_manager_p m)
{
	size_t i;
	int retval = 0;
	int receipted;
	mtsched_p s = m->s_ref;
	
	do
	{
		receipted = 0;
		rwlock_lock_w(&m->l);
		for(i = 0; i < m->num_joblist; i++)
		{
			if(mtsched_query_job_status(s, m->joblist[i]) == qs_finished)
			{
				uint32_t job_id = m->joblist[i];
				mtsched_receipt_job(s, job_id);
				m->joblist[i] = m->joblist[--m->num_joblist];
				receipted = 1;
				retval = 1;
#if MTSCHEDMAN_LOG_EVENTS >= 3
				logprintf("[mtschedman] Receipted Job %08x.\n", job_id);
#endif
				break;
			}
		}
		rwlock_unlock_w(&m->l);
	}while(receipted);
	
	return retval;
}

int mtschedman_dispatch(mtsched_manager_p m, mtsched_proc proc, size_t max_args, ...)
{
	mtsched_p s;
	backoff_t bo;
	int locked = 0;
	mtsched_form_t f;
	va_list ap;
	size_t i;
	uint32_t job_id;
	int bo_init = 0;
	
	if(!m) goto InvalidParamExit;
	
	s = m->s_ref;
	
	if(!mtsched_initform(s, &f, proc, 0)) goto FailExit;
	f.max_params = max_args;
	va_start(ap, max_args);
	for(i = 0; i < max_args; i++) f.params[i] = va_arg(ap, ptrdiff_t);
	va_end(ap);
	
	for(;;)
	{
		mtschedman_receipt_finished_jobs(m);
		rwlock_lock_w(&m->l); locked = 1;
		if(m->num_joblist < m->max_joblist && mtsched_submit(&f, &job_id))
		{
#if MTSCHEDMAN_LOG_EVENTS >= 3
			logprintf("[mtschedman] Dispatched new job %08x.\n", job_id);
#endif
			m->joblist[m->num_joblist++] = job_id;
			break;
		}
		else
		{
			if(!bo_init)
			{
				backoff_init(&bo, s->max_pause_inst, s->max_wakeup_delay);
#if MTSCHEDMAN_LOG_EVENTS >= 2
				logprintf("[mtschedman] Job manager full, waiting for reception of a job.\n");
#endif
				bo_init = 1;
			}
			backoff_update(&bo);
		}
		rwlock_unlock_w(&m->l); locked = 0;
	}
	if (locked) rwlock_unlock_w(&m->l);
#if MTSCHEDMAN_LOG_EVENTS >= 3
	logprintf("[mtschedman] A new job had been dispatched as %08x.\n", job_id);
#endif

	return 1;
InvalidParamExit:
	logprintf_e("[mtschedman] Failed to dispatch a job: Invalid parameters.\n");
	goto FailExit;
FailExit:
	if(locked) rwlock_unlock_w(&m->l);
	return 0;
}

int mtschedman_finish(mtsched_manager_p m)
{
	mtsched_p s;
	backoff_t bo;
	
	if(!m) goto InvalidParamExit;
	
	s = m->s_ref;

#if MTSCHEDMAN_LOG_EVENTS >= 3
	if(m->num_joblist)
		logprintf("[mtschedman] Finishing all jobs.\n");
#endif
	
	backoff_init(&bo, s->max_pause_inst, s->max_wakeup_delay);
	for(;m->num_joblist; backoff_update(&bo))
	{
		mtschedman_receipt_finished_jobs(m);
	}
	
	return 1;
InvalidParamExit:
	logprintf_e("[mtschedman] Finishing all jobs failed: Invalid parameters.\n");
	goto FailExit;
FailExit:
	return 0;
}

void mtschedman_free(mtsched_manager_p m)
{
	if(!m) return;
	
	if(m->joblist)
	{
		mtschedman_finish(m);
		free(m->joblist);
	}
	free(m);
}
