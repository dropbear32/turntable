/*
    turntable - client-to-server audio streaming
    Copyright (c) 2021-2022 Ally Sommers

    This code is licensed under the BSD 3-Clause License. A copy of this
    license is included with this code.
*/

#ifndef TURNTABLE_PTHREAD_H
#define TURNTABLE_PTHREAD_H

#ifdef _WIN32

#include <Windows.h>

typedef HANDLE pthread_t;
typedef CRITICAL_SECTION pthread_mutex_t;
typedef CONDITION_VARIABLE pthread_cond_t;

typedef void pthread_attr_t;
typedef void pthread_mutexattr_t;
typedef void pthread_condattr_t;

int pthread_create(pthread_t *thread, const pthread_attr_t *restrict attr, void *(*start_routine)(void *), void *arg);

int pthread_mutex_init(pthread_mutex_t *restrict mutex, const pthread_mutexattr_t *attr);
int pthread_mutex_lock(pthread_mutex_t *mutex);
int pthread_mutex_unlock(pthread_mutex_t *mutex);

int pthread_cond_init(pthread_cond_t *restrict cond, const pthread_condattr_t *restrict attr);
int pthread_cond_wait(pthread_cond_t *restrict cond, pthread_mutex_t *restrict mutex);
int pthread_cond_signal(pthread_cond_t *cond);

#else
#include <pthread.h>
#endif

#endif /* TURNTABLE_PTHREAD_H */
