#ifndef _MTSCHED_H_
#define _MTSCHED_H_ 1

#include<stdint.h>
#include<stddef.h>

#include<mtutil/mtcommon.h>
#include<mtutil/mutex.h>
#include<mtutil/rwlock.h>
#include<avlbst/avlbst.h>

#define MTSCHED_MAX_PARAMS 128
#define MTACCESS volatile

typedef int(*mtsched_proc)(ptrdiff_t param1, ...);

typedef struct mtsched_queue_struct
{
	atomic_int status;
	MTACCESS uint32_t job_id;
	MTACCESS size_t max_params;
	MTACCESS ptrdiff_t params[MTSCHED_MAX_PARAMS];
	MTACCESS void *mtsched_func;
	MTACCESS int returned_value;
	MTACCESS uint32_t options;
}mtsched_queue_t, *mtsched_queue_p;

// Values for options field
#define mtsched_default			0 // The default options for the job
#define mtsched_not_monitored	1 // The job doesn't need to be receipted

typedef enum queue_status_t
{
	qs_vacant = 0,
	qs_init,
	qs_pending,
	qs_processing,
	qs_finished
}queue_status_t, *queue_status_p;

typedef struct mtsched_struct
{
	// Job id assigner
	MTACCESS uint32_t cur_job_id;
	
	// Jobs queue
	size_t max_jobqueue;
	mtsched_queue_p jobqueue; // Allocated on creation.
	size_t num_jobs_per_thread; // Usually 2 to 4
	avlbst_p jobqueue_avlbst;
	rwlock_t jobqueue_avlbst_lock;
	
	// Running status
	MTACCESS size_t num_processing_jobs;
	MTACCESS size_t num_pending_jobs;
	MTACCESS size_t num_working_threads;
	
	// Tuning parameters, don't change them if you don't know much about them
	size_t stack_size;
	MTACCESS unsigned max_pause_inst;
	MTACCESS unsigned max_wakeup_delay; // In ms. The larger number, the less CPU overhead
	
	MTACCESS int quit; // All quit
	
	// Number of thread handles
	thread_handle_p thread_handles;
	size_t num_thread_handles;
	
	// Miscellaneous
	void *userdata;
}mtsched_t, *mtsched_p;

typedef struct mtsched_form_struct
{
	mtsched_p s;
	size_t max_params;
	ptrdiff_t params[MTSCHED_MAX_PARAMS];
	uint32_t options;
	void *mtsched_func;
}mtsched_form_t, *mtsched_form_p;

//=============================================================================
// Func: mtsched_get_sys_logic_threads
// Desc: Get the number of system logical threads.
//-----------------------------------------------------------------------------
int mtsched_get_sys_logic_threads();

//=============================================================================
// Func: mtsched_create
// Desc: Create a multithread scheduler.
// Params:
//   logic_threads: The number of the logical threads. Should be the actual
//     number of the CPU threads. Set to 0 means automatically determine the
//     threads to be created.
// Return value:
//   A mtsched instance, for further usage.
//-----------------------------------------------------------------------------
mtsched_p mtsched_create(int logic_threads, size_t num_jobs_per_thread, size_t stack_size, void *userdata);

//=============================================================================
// Func: mtsched_get_numthreads
// Desc: Get number of the worker threads of a scheduler. 
// Return value:
//   The function returns the count of the worker threads, which also includes
//   the current thread. If the function fails, the return value is zero.
//-----------------------------------------------------------------------------
size_t mtsched_get_numthreads(mtsched_p s);

//=============================================================================
// Func: mtsched_initform
// Desc: Initialize a form for multithreading. The form includes a function
//   pointer for doing the job, some parameters for calling that function,
//   and something else.
// Params:
//   s: The created scheduler.
//   f: A pointer to the form that describes how to let the worker helps with
//     co-working. The form can be stored on the stack, TLS, or whatever else.
//     The form doesn't need to be accessed by other thread so it doesn't
//     require a malloc() invocation to store it on the heap.
//   proc: A function pointer that does the job. Be careful that the function
//     will be invoked by other thread, which doesn't share the same stack and
//     TLS area with the current thread, only global variables or allocated
//     memory which is on the heap can be accessed by other thread.
//   options: The options for the job.
// Return value:
//   Non-zero for success, zero for failure.
//-----------------------------------------------------------------------------
int mtsched_initform(mtsched_p s, mtsched_form_p f, mtsched_proc proc, uint32_t options);

//=============================================================================
// Func: mtsched_setparams
// Desc: Set parameters for the worker to call the specific function with
//   proper arguments.
// Params:
//   form: The form that describes how to process the job.
//   max_args: Number of parameters
//   ...: Parameters that will be passed to the procedure. *NOTE* that all of
//     the float type parameters will be casted to double on MSVC, so please
//     use double type instead of float.
// Return value:
//   Non-zero for success, zero for failure.
//-----------------------------------------------------------------------------
int mtsched_setparams(mtsched_form_p form, size_t max_args, ...);

//=============================================================================
// Func: mtsched_submit
// Desc: Submit the form to the scheduler. The scheduler let the co-workers
//   process the job.
// Params:
//   form: The form that describes how to process the job, Including parameter
//     list, a function pointer, and something else.
//   out_job_id: A pointer to retrieve the job id, in which you can use it to
//     monitor the job and receipt it when done.
// Return value:
//   Non-zero for success, zero for failure.
//-----------------------------------------------------------------------------
int mtsched_submit(mtsched_form_p form, uint32_t *out_job_id);

//=============================================================================
// Func: mtsched_participate
// Desc: Let the current thread process a job. If there's no jobs to do, the
//   function returns zero. Otherwise a non-zero value will be returned.
//-----------------------------------------------------------------------------
int mtsched_participate(mtsched_p s);

//=============================================================================
// Func: mtsched_query_job_status
// Desc: Query the job status.
//-----------------------------------------------------------------------------
queue_status_t mtsched_query_job_status(mtsched_p s, uint32_t job_id);

//=============================================================================
// Func: mtsched_get_retval
// Desc: Get the return value of a job, if it's finished.
// Return value:
//   Non-zero for success (the job is finished and returned a value),
//   zero for failure (the job is pending or processing).
//-----------------------------------------------------------------------------
int mtsched_get_retval(mtsched_p s, uint32_t job_id, int *pret);

//=============================================================================
// Func: mtsched_receipt_job
// Desc: Receipt the job, so the job_id can be reused.
//-----------------------------------------------------------------------------
int mtsched_receipt_job(mtsched_p s, uint32_t job_id);

//=============================================================================
// Func: mtsched_free
// Desc: Free the scheduler, wait for all of the co-workers quit, then return.
//-----------------------------------------------------------------------------
void mtsched_free(mtsched_p s);

#endif
