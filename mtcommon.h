#pragma once

#include"mtatomic.h"
#include<stdint.h>
#include<stddef.h>

#ifdef _WIN32
#  include<Windows.h>
#endif

#ifdef _MSC_VER
#  include<xmmintrin.h>
#  define TLS_VAR __declspec(thread)
#  define mt_cpu_relax() _mm_pause()
#elif defined(__GNUC__) || defined(__clang__) // !_MSC_VER
#  define TLS_VAR __thread
#  define mt_cpu_relax() asm volatile("PAUSE")
#endif // !_MSC_VER

#if __STDC_VERSION__ >= 201112L && HAVE_THREADS_H
#  include<threads.h>
#  define TLS_VAR thread_local
#endif // !(__STDC_VERSION__ >= 201112L && HAVE_THREADS_H)

#if !MT_ENABLED && !defined(TLS_VAR)
#define TLS_VAR
#endif // !(!MT_ENABLED && !defined(TLS_VAR))

#define MAX_WAIT_ITERS 0x40000
#define MAX_BACKOFF_ITERS 0x10000
#define MAX_BO_SLEEPS 20

void mt_sleep(unsigned sleeps);

#if !defined(_WIN32)
#  define MT_USE_PTHREAD 1
#  include<pthread.h>
#endif // !_WIN32

typedef struct thread_info_struct
{
#ifdef _WIN32
	HANDLE h;
#elif MT_USE_PTHREAD
	pthread_t p;
#endif // !MT_USE_PTHREAD
	int(*thread_proc)(void *param);
	void *param;
	volatile int running;
	volatile int return_value;
}thread_info_t, *thread_info_p;
typedef thread_info_p thread_handle_t, *thread_handle_p;

//=============================================================================
// Func: mt_create_thread
// Desc: Create a logical thread.
// Params:
//   stack_size: The size of the stack of the new thread. If it's zero, the
//     default amount of the stack memory will be allocated for the new thread.
//   thread_proc: A function pointer that defines the behavior of the new
//     thread. The new thread may start immediately before the function
//     returns.
//   param: A parameter that will be passed to the thread function.
// Return value:
//   A thread handle that can be used to monitor or control the thread.
//-----------------------------------------------------------------------------
thread_handle_t mt_create_thread(size_t stack_size, int(*thread_proc)(void *param), void *param);

//=============================================================================
// Func: mt_get_return_value
// Desc: Get the return value of the thread.
// Params:
//   th: The thread handler
//   pret: A pointer used to retrieve the return value of the thread.
// Return value:
//   Non-zero for success, zero for failure. NOTE that the return value of this
//   function is NOT the return value of the thread function.
//   If the thread function is still running, the function fails.
//-----------------------------------------------------------------------------
int mt_get_return_value(thread_handle_t th, int *pret);

//=============================================================================
// Func: mt_free
// Desc: Wait until a thread quits, and then free a thread handler.
//-----------------------------------------------------------------------------
void mt_free(thread_handle_t th);

//=============================================================================
// Backoff feature: Optimizing multithread contention logic.
//   This feature works as a light-weight yielding operation, and generally be
//   used on spin loops. The backoff time grows exponentially.
//-----------------------------------------------------------------------------
typedef struct backoff_struct
{
	unsigned relaxes;
	unsigned sleeps;
	unsigned max_relaxes;
	unsigned max_sleeps;
}backoff_t, *backoff_p;

//=============================================================================
// Func: backoff_init
// Desc: Initialize for backoff feature
// Params:
//   bo: The backoff state data.
//   max_relaxes: Loop how many PAUSE instructions (or anything else) cycles.
//     The times of the cycle grows exponentially, until exceed max_relaxes.
//     When it exceeded max_relaxes, the backoff feature sleeps as the logic of
//     heavy-weight yielding.
//   max_sleeps: How many milliseconds it sleeps. This value defines themaximum
//     tolerable wakeup delay of a sleeping thread. The larger it is, the less
//    CPU usage the thread costs.
//-----------------------------------------------------------------------------
void backoff_init(backoff_p bo, unsigned max_relaxes, unsigned max_sleeps);

//=============================================================================
// Func: backoff_update
// Desc: Update the state of the backoff feature, then do backoff yielding.
// Return value:
//   Zero if using PAUSE instructions to do backoff yielding, non-zero if using
//   sleep to do backoff yielding.
//-----------------------------------------------------------------------------
int backoff_update(backoff_p bo);
