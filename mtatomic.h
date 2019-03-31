#pragma once

#if defined(__GNUC__) && !defined(_GCC_VER)
#  define _GCC_VER __GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__
#endif

#ifndef STDATOMIC_SELF_IMPLEMENTATION
#  if defined(__GNUC__) 
#    if _GCC_VER >= 40900 && !defined(__STDC_NO_ATOMICS__)
#      include<stdatomic.h>
#    else
#      define STDATOMIC_SELF_IMPLEMENTATION 1
#    endif
#  endif
#  ifdef _MSC_VER
#    define STDATOMIC_SELF_IMPLEMENTATION 1
#  endif
#  ifdef __clang__
#    include<stdatomic.h>
#  endif
#endif

#include<stddef.h>
#include<stdint.h>

#if defined(_WIN32) && STDATOMIC_SELF_IMPLEMENTATION == 1
#  include<Windows.h>

typedef volatile intptr_t atomic_flag;
typedef volatile intptr_t atomic_bool;
typedef volatile intptr_t atomic_char;
typedef volatile intptr_t atomic_schar;
typedef volatile intptr_t atomic_uchar;
typedef volatile intptr_t atomic_short;
typedef volatile intptr_t atomic_ushort;
typedef volatile intptr_t atomic_int;
typedef volatile intptr_t atomic_uint;
typedef volatile intptr_t atomic_long;
typedef volatile intptr_t atomic_ulong;
typedef volatile intptr_t atomic_llong;
typedef volatile intptr_t atomic_ullong;
typedef volatile intptr_t atomic_wchar_t;
typedef volatile intptr_t atomic_int_least8_t;
typedef volatile intptr_t atomic_uint_least8_t;
typedef volatile intptr_t atomic_int_least16_t;
typedef volatile intptr_t atomic_uint_least16_t;
typedef volatile intptr_t atomic_int_least32_t;
typedef volatile intptr_t atomic_uint_least32_t;
typedef volatile intptr_t atomic_int_least64_t;
typedef volatile intptr_t atomic_uint_least64_t;
typedef volatile intptr_t atomic_int_fast8_t;
typedef volatile intptr_t atomic_uint_fast8_t;
typedef volatile intptr_t atomic_int_fast16_t;
typedef volatile intptr_t atomic_uint_fast16_t;
typedef volatile intptr_t atomic_int_fast32_t;
typedef volatile intptr_t atomic_uint_fast32_t;
typedef volatile intptr_t atomic_int_fast64_t;
typedef volatile intptr_t atomic_uint_fast64_t;
typedef volatile intptr_t atomic_intptr_t;
typedef volatile intptr_t atomic_uintptr_t;
typedef volatile intptr_t atomic_size_t;
typedef volatile intptr_t atomic_ptrdiff_t;
typedef volatile intptr_t atomic_intmax_t;
typedef volatile intptr_t atomic_uintmax_t;

#define ATOMIC_FLAG_INIT 0

#define ATOMIC_VAR_INIT(value) (value)

#define atomic_init(obj, value) \
do {                            \
    *(obj) = (value);           \
} while(0)

#define kill_dependency(y) ((void)0)

#define atomic_thread_fence(order) \
    MemoryBarrier()

#define atomic_signal_fence(order) \
    ((void)0)

#define atomic_is_lock_free(obj) 0

#define atomic_store(object, desired)   \
do {                                    \
    *(object) = (desired);              \
    MemoryBarrier();                    \
} while (0)

#define atomic_store_explicit(object, desired, order) \
    atomic_store(object, desired)

#define atomic_load(object) \
    (MemoryBarrier(), *(object))

#define atomic_load_explicit(object, order) \
    atomic_load(object)

#define atomic_exchange(object, desired) \
    InterlockedExchangePointer(object, desired);

#define atomic_exchange_explicit(object, desired, order) \
    atomic_exchange(object, desired)

static __inline int atomic_compare_exchange_strong(volatile void *object, volatile void *expected,
	intptr_t desired)
{
	intptr_t old = *(intptr_t*)expected;
	*(intptr_t*)expected = (intptr_t)InterlockedCompareExchangePointer((volatile PVOID*)object, (PVOID)desired, (PVOID)old);
	return *(intptr_t*)expected == old;
}

#define atomic_compare_exchange_strong_explicit(object, expected, desired, success, failure) \
    atomic_compare_exchange_strong(object, expected, desired)

#define atomic_compare_exchange_weak(object, expected, desired) \
    atomic_compare_exchange_strong(object, expected, desired)

#define atomic_compare_exchange_weak_explicit(object, expected, desired, success, failure) \
    atomic_compare_exchange_weak(object, expected, desired)

#ifdef _WIN64
#define atomic_fetch_add(object, operand) \
    InterlockedExchangeAdd64(object, operand)

#define atomic_fetch_sub(object, operand) \
    InterlockedExchangeAdd64(object, -(operand))

#define atomic_fetch_or(object, operand) \
    InterlockedOr64(object, operand)

#define atomic_fetch_xor(object, operand) \
    InterlockedXor64(object, operand)

#define atomic_fetch_and(object, operand) \
    InterlockedAnd64(object, operand)
#else
#define atomic_fetch_add(object, operand) \
    InterlockedExchangeAdd(object, operand)

#define atomic_fetch_sub(object, operand) \
    InterlockedExchangeAdd(object, -(operand))

#define atomic_fetch_or(object, operand) \
    InterlockedOr(object, operand)

#define atomic_fetch_xor(object, operand) \
    InterlockedXor(object, operand)

#define atomic_fetch_and(object, operand) \
    InterlockedAnd(object, operand)
#endif /* _WIN64 */

#define atomic_fetch_add_explicit(object, operand, order) \
    atomic_fetch_add(object, operand)

#define atomic_fetch_sub_explicit(object, operand, order) \
    atomic_fetch_sub(object, operand)

#define atomic_fetch_or_explicit(object, operand, order) \
    atomic_fetch_or(object, operand)

#define atomic_fetch_xor_explicit(object, operand, order) \
    atomic_fetch_xor(object, operand)

#define atomic_fetch_and_explicit(object, operand, order) \
    atomic_fetch_and(object, operand)

#define atomic_flag_test_and_set(object) \
    atomic_exchange(object, 1)

#define atomic_flag_test_and_set_explicit(object, order) \
    atomic_flag_test_and_set(object)

#define atomic_flag_clear(object) \
    atomic_store(object, 0)

#define atomic_flag_clear_explicit(object, order) \
    atomic_flag_clear(object)

#  undef STDATOMIC_SELF_IMPLEMENTATION
#  define STDATOMIC_SELF_IMPLEMENTATION 2
#endif // defined(_WIN32) && STDATOMIC_SELF_IMPLEMENTATION == 1

#if defined(__GNUC__) && STDATOMIC_SELF_IMPLEMENTATION == 1

typedef volatile int			atomic_flag;
typedef volatile int			atomic_bool;
typedef volatile char			atomic_char;
typedef volatile signed char 	atomic_schar;
typedef volatile unsigned char 	atomic_uchar;
typedef volatile short 			atomic_short;
typedef volatile unsigned short	atomic_ushort;
typedef volatile int 			atomic_int;
typedef volatile unsigned int 	atomic_uint;
typedef volatile long 			atomic_long;
typedef volatile unsigned long 	atomic_ulong;
typedef volatile long long 		atomic_llong;
typedef volatile unsigned long long atomic_ullong;
typedef volatile wchar_t 		atomic_wchar_t;
typedef volatile int_least8_t	atomic_int_least8_t;
typedef volatile uint_least8_t	atomic_uint_least8_t;
typedef volatile int_least16_t	atomic_int_least16_t;
typedef volatile uint_least16_t	atomic_uint_least16_t;
typedef volatile int_least32_t	atomic_int_least32_t;
typedef volatile uint_least32_t	atomic_uint_least32_t;
typedef volatile int_least64_t	atomic_int_least64_t;
typedef volatile uint_least64_t	atomic_uint_least64_t;
typedef volatile int_fast8_t	atomic_int_fast8_t;
typedef volatile uint_fast8_t	atomic_uint_fast8_t;
typedef volatile int_fast16_t	atomic_int_fast16_t;
typedef volatile uint_fast16_t	atomic_uint_fast16_t;
typedef volatile int_fast32_t	atomic_int_fast32_t;
typedef volatile uint_fast32_t	atomic_uint_fast32_t;
typedef volatile int_fast64_t	atomic_int_fast64_t;
typedef volatile uint_fast64_t	atomic_uint_fast64_t;
typedef volatile intptr_t		atomic_intptr_t;
typedef volatile uintptr_t		atomic_uintptr_t;
typedef volatile size_t			atomic_size_t;
typedef volatile ptrdiff_t		atomic_ptrdiff_t;
typedef volatile intmax_t		atomic_intmax_t;
typedef volatile uintmax_t		atomic_uintmax_t;

#define ATOMIC_FLAG_INIT 0

#define ATOMIC_VAR_INIT(value) (value)

#define atomic_init(obj, value)  __sync_lock_test_and_set(obj, value)

#define kill_dependency(y) ((void)0)

#define atomic_thread_fence(order) \
    __sync_synchronize()

#define atomic_signal_fence(order) \
    __sync_synchronize()

#define atomic_is_lock_free(obj) 0

#define atomic_store(object, desired) __sync_lock_test_and_set(object, desired)

#define atomic_store_explicit(object, desired, order) \
    atomic_store(object, desired)

#define atomic_load(object) \
    (__sync_synchronize(), *(object))

#define atomic_load_explicit(object, order) \
    atomic_load(object)

#define atomic_exchange(object, desired) \
    __sync_lock_test_and_set(object, desired);

#define atomic_exchange_explicit(object, desired, order) \
    atomic_exchange(object, desired)

#define atomic_compare_exchange_strong(object, expected, desired) __sync_bool_compare_and_swap(object, *expected, desired)

#define atomic_compare_exchange_strong_explicit(object, expected, desired, success, failure) \
    atomic_compare_exchange_strong(object, expected, desired)

#define atomic_compare_exchange_weak(object, expected, desired) \
    atomic_compare_exchange_strong(object, expected, desired)

#define atomic_compare_exchange_weak_explicit(object, expected, desired, success, failure) \
    atomic_compare_exchange_weak(object, expected, desired)

#define atomic_fetch_add(object, operand) \
    __sync_fetch_and_add(object, operand)

#define atomic_fetch_sub(object, operand) \
    __sync_fetch_and_sub(object, -(operand))

#define atomic_fetch_or(object, operand) \
    __sync_fetch_and_or(object, operand)

#define atomic_fetch_xor(object, operand) \
    __sync_fetch_and_xor(object, operand)

#define atomic_fetch_and(object, operand) \
    __sync_fetch_and_and(object, operand)

#define atomic_fetch_add_explicit(object, operand, order) \
    atomic_fetch_add(object, operand)

#define atomic_fetch_sub_explicit(object, operand, order) \
    atomic_fetch_sub(object, operand)

#define atomic_fetch_or_explicit(object, operand, order) \
    atomic_fetch_or(object, operand)

#define atomic_fetch_xor_explicit(object, operand, order) \
    atomic_fetch_xor(object, operand)

#define atomic_fetch_and_explicit(object, operand, order) \
    atomic_fetch_and(object, operand)

#define atomic_flag_test_and_set(object) \
    atomic_exchange(object, 1)

#define atomic_flag_test_and_set_explicit(object, order) \
    atomic_flag_test_and_set(object)

#define atomic_flag_clear(object) \
    atomic_store(object, 0)

#define atomic_flag_clear_explicit(object, order) \
    atomic_flag_clear(object)

#  undef STDATOMIC_SELF_IMPLEMENTATION
#  define STDATOMIC_SELF_IMPLEMENTATION 2
#endif // defined(__GNUC__) && STDATOMIC_SELF_IMPLEMENTATION == 1
