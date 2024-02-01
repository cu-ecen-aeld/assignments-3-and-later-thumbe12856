#include "threading.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

// Optional: use these functions to add debug or error prints to your application
#define DEBUG_LOG(msg,...) printf("threading: " msg "\n" , ##__VA_ARGS__)
#define ERROR_LOG(msg,...) printf("threading ERROR: " msg "\n" , ##__VA_ARGS__)

void* threadfunc(void* thread_param)
{
    // wait, obtain mutex, wait, release mutex as described by thread_data structure
    struct thread_data* thread_func_args = (struct thread_data*)thread_param;

    int res = -1;
    // wait for required msec to obtain the mutex
    res = usleep(thread_func_args->wait_to_obtain_us);

    if (res != 0) {
        ERROR_LOG("Failed to sleep to wait to obtain mutex.");
    }

    pthread_mutex_lock(thread_func_args->mutex);

    res = usleep(thread_func_args->wait_to_release_us);
    if (res != 0) {
        ERROR_LOG("Failed to sleep to wait to release mutex.");
    }

    res = pthread_mutex_unlock(thread_func_args->mutex);
    if (res == 0) {
        thread_func_args->thread_complete_success = true;
    }
    else {
        thread_func_args->thread_complete_success = false;
        ERROR_LOG("Failed to pthread_mutex_unlock.");
    }

    DEBUG_LOG("thread_func_args->thread_complete_success: %d", thread_func_args->thread_complete_success);

    return ((void*)thread_func_args);
}


bool start_thread_obtaining_mutex(pthread_t* thread, pthread_mutex_t* mutex, int wait_to_obtain_ms, int wait_to_release_ms)
{
    /**
     * TODO: allocate memory for thread_data, setup mutex and wait arguments, pass thread_data to created thread
     * using threadfunc() as entry point.
     *
     * return true if successful.
     *
     * See implementation details in threading.h file comment block
     */

    struct thread_data* thread_data_instance = malloc(sizeof(struct thread_data*));
    if (thread_data_instance == NULL) {
        ERROR_LOG("Malloc memory error.");
        return false;
    }

    thread_data_instance->mutex = mutex;
    thread_data_instance->wait_to_obtain_us = wait_to_obtain_ms * 1000;
    thread_data_instance->wait_to_release_us = wait_to_release_ms * 1000;

    int rc = pthread_create(thread, NULL, threadfunc, (void*)thread_data_instance);
    if (rc == 0) {
        return true;
    } else {
        return false;
    }
}
