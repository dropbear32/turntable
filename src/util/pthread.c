/*
    turntable - client-to-server audio streaming
    Copyright (c) 2021-2022 Ally Sommers

    This code is licensed under the BSD 3-Clause License. A copy of this
    license is included with this code.
*/

#include "pthread.h"

#ifdef _WIN32

int pthread_create(pthread_t *thread, const pthread_attr_t *restrict attr, void *(*start_routine)(void *), void *arg) {
    (void)attr;

    if (thread == NULL || start_routine == NULL)
        return 1;

    *thread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)start_routine, arg, 0, NULL);
    return *thread == NULL;
}

int pthread_mutex_init(pthread_mutex_t *restrict mutex, const pthread_mutexattr_t *attr) {
    (void)attr;

    if (mutex == NULL)
        return 1;

    InitializeCriticalSection(mutex);
    return 0;
}

int pthread_mutex_lock(pthread_mutex_t *mutex) {
    if (mutex == NULL)
        return 1;

    EnterCriticalSection(mutex);
    return 0;
}

int pthread_mutex_unlock(pthread_mutex_t *mutex) {
    if (mutex == NULL)
        return 1;

    LeaveCriticalSection(mutex);
    return 0;
}

int pthread_cond_init(pthread_cond_t *restrict cond, const pthread_condattr_t *restrict attr) {
    (void)attr;
    if (cond == NULL)
        return 1;

    InitializeConditionVariable(cond);
    return 0;
}

int pthread_cond_wait(pthread_cond_t *restrict cond, pthread_mutex_t *restrict mutex) {
    if (cond == NULL || mutex == NULL)
        return 1;

    while (!SleepConditionVariableCS(cond, mutex, 0))
        continue;
    return 0;
}
int pthread_cond_signal(pthread_cond_t *cond) {
    if (cond == NULL)
        return 1;

    WakeConditionVariable(cond);
    return 0;
}

#endif
