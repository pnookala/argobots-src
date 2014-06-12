/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 * See COPYRIGHT in top-level directory.
 */

#include <stdlib.h>
#include <string.h>
#include "abti.h"

/** @defgroup Future Future
 * Futures are used to wait for values asynchronously.
 */

/**
 * @ingroup Future
 * @brief   Blocks current thread until the feature has finished its computation.
 * 
 * @param[in]  future       Reference to the future
 * @param[out] value        Reference to value of future
 * @return Error code
 */
int ABT_future_wait(ABT_future future, void **value)
{
	int abt_errno = ABT_SUCCESS;
	ABTI_future *p_future = ABTI_future_get_ptr(future);	
    if (!p_future->ready) {
        ABTI_thread_entry *cur = (ABTI_thread_entry*) ABTU_malloc(sizeof(ABTI_thread_entry));
        cur->current = ABTI_thread_current();
        cur->next = NULL;
        if(p_future->waiters.tail != NULL)
            p_future->waiters.tail->next = cur;
        p_future->waiters.tail = cur;
        if(p_future->waiters.head == NULL)
            p_future->waiters.head = cur;
        ABTI_thread_suspend();
    }
    *value = p_future->value;
	return abt_errno;
}

/**
 * @ingroup Future
 * @brief   Signals all threads blocking on a future once the result has been calculated.
 * 
 * @param[in]  data       Pointer to future's data
 * @return No value returned
 */
void ABTI_future_signal(ABTI_future *p_future)
{
    ABTI_thread_entry *cur = p_future->waiters.head;
    while(cur!=NULL)
    {
        ABT_thread mythread = cur->current;
        ABTI_thread_set_ready(mythread);
        ABTI_thread_entry *tmp = cur;
        cur=cur->next;
        free(tmp);
    }
}

/**
 * @ingroup Future
 * @brief   Sets a nbytes-value into a future for all threads blocking on the future to resume.
 * 
 * @param[in]  future       Reference to the future
 * @param[in]  value        Pointer to the buffer containing the result
 * @param[in]  nbytes       Number of bytes in the buffer
 * @return Error code
 */
int ABT_future_set(ABT_future future, void *value, int nbytes)
{
	int abt_errno = ABT_SUCCESS;
	ABTI_future *p_future = ABTI_future_get_ptr(future);	
    p_future->ready = 1;
    memcpy(p_future->value, value, nbytes);
    ABTI_future_signal(p_future);
	return abt_errno;
}

/**
 * @ingroup Future
 * @brief   Creates a future.
 * 
 * @param[in]  n            Number of bytes in the buffer containing the result of the future
 * @param[in]  xstream      ES on which this future will run
 * @param[out] future       Reference to the newly created future
 * @return Error code
 */
int ABT_future_create(int n, ABT_xstream xstream, ABT_future *future)
{
	int abt_errno = ABT_SUCCESS;
    ABTI_future *p_future = (ABTI_future*)ABTU_malloc(sizeof(ABTI_future));
    p_future->xstream = xstream;
    p_future->ready = 0;
    p_future->nbytes = n;
    p_future->value = ABTU_malloc(n);
    p_future->waiters.head = p_future->waiters.tail = NULL;
    *future = p_future;
	return abt_errno;
}

/**
 * @ingroup Future
 * @brief   Releases the memory of a future.
 * 
 * @param[out] future       Reference to the future
 * @return Error code
 */
int ABT_future_free(ABT_future *future)
{
	int abt_errno = ABT_SUCCESS;
	ABTI_future *p_future = ABTI_future_get_ptr(*future);
	ABTU_free(p_future->value);
	ABTU_free(p_future);
	return abt_errno;
}

/* Private API */
ABTI_future *ABTI_future_get_ptr(ABT_future future)
{
    ABTI_future *p_future;
    if (future == ABT_FUTURE_NULL) {
        p_future = NULL;
    } else {
        p_future = (ABTI_future *)future;
    }
    return p_future;
}