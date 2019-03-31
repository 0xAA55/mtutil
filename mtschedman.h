#pragma once
#include<stdint.h>
#include<stddef.h>
#include"mtsched.h"
#include"rwlock.h"
#include"avlbst.h"

//=============================================================================
// mtsched_manager feature: The main purpose of this feature is to manage every
//   single job, simplify management of jobs, and let the parallel computing
//   more effective.
//-----------------------------------------------------------------------------
typedef struct mtsched_manager_struct
{
	rwlock_t l;
	mtsched_p s_ref;
	uint32_t *joblist;
	size_t num_joblist;
	size_t max_joblist;
}mtsched_manager_t, *mtsched_manager_p;

//=============================================================================
// Func: mtschedman_create
// Desc: Create a job manager by referencing a mtsched instance.
//-----------------------------------------------------------------------------
mtsched_manager_p mtschedman_create(mtsched_p s);

//=============================================================================
// Func: mtschedman_dispatch
// Desc: Dispatch a job to the scheduler and record the job_id in order to
//   monitor the job.
// Params:
//   m: The job manager.
//   proc: The function entry point.
// Return value:
//   Non-zero for success, zero for failure.
//-----------------------------------------------------------------------------
int mtschedman_dispatch(mtsched_manager_p m, mtsched_proc proc, size_t max_args, ...);

//=============================================================================
// Func: mtschedman_finish
// Desc: Wait for all of the jobs that were dispatched by the manager finish.
// Return value:
//   Non-zero for success, zero for failure.
//-----------------------------------------------------------------------------
int mtschedman_finish(mtsched_manager_p m);

//=============================================================================
// Func: mtschedman_free
// Desc: Wait for all of the jobs that were dispatched by the manager finish,
//   then cleanup memory usage.
//-----------------------------------------------------------------------------
void mtschedman_free(mtsched_manager_p m);

