/*
 * Implementation of locks.h
 *
 * Matthew Atkin
 * s3603797
 *
 * Sept 2019
 */
#include <pthread.h>
#include "locks.h"

int rw_lock_init(struct rw_lock_t* rw_lock)
{
    rw_lock->readers = 0;
    rw_lock->writing = 0;
    rw_lock->readers_waiting = 0;
    rw_lock->writers_waiting = 0;

    int result = pthread_cond_init(&rw_lock->signal_readers, NULL);
    if(result)
        return result;
    result = pthread_cond_init(&rw_lock->signal_writers, NULL);
    if(result)
        return result;
    
    // Pass the return value back through
    return pthread_mutex_init(&rw_lock->internal_lock, NULL);
}

int rw_lock_destroy(struct rw_lock_t* rw_lock)
{
    int result = pthread_cond_destroy(&rw_lock->signal_readers);
    if(result)
        return result;
    result = pthread_cond_destroy(&rw_lock->signal_writers);
    if(result)
        return result;

    // Pass the return value back through
    return pthread_mutex_destroy(&rw_lock->internal_lock);
}

int r_lock(struct rw_lock_t* rw_lock)
{
    pthread_mutex_lock(&rw_lock->internal_lock);
    
    if(rw_lock->writing || rw_lock->writers_waiting > 0)
    {
        rw_lock->readers_waiting++;
        do
        {
            pthread_cond_wait(&rw_lock->signal_readers, &rw_lock->internal_lock);
        }
        while(rw_lock->writing || rw_lock->writers_waiting > 0);
        rw_lock->readers_waiting--;
    }

    rw_lock->readers++;

    pthread_mutex_unlock(&rw_lock->internal_lock);
    return 0;
}

int r_unlock(struct rw_lock_t* rw_lock)
{
    pthread_mutex_lock(&rw_lock->internal_lock);
    
    rw_lock->readers--;

    if(rw_lock->readers == 0 && rw_lock->writers_waiting > 0)
    {
        pthread_cond_signal(&rw_lock->signal_writers);
    }

    pthread_mutex_unlock(&rw_lock->internal_lock);
    return 0;
}

int w_lock(struct rw_lock_t* rw_lock)
{
    pthread_mutex_lock(&rw_lock->internal_lock);
    
    if(rw_lock->writing)
    {
        rw_lock->writers_waiting++;
        do
        {
            pthread_cond_wait(&rw_lock->signal_writers, &rw_lock->internal_lock);
        }
        while(rw_lock->writing);
        rw_lock->writers_waiting--;
    }

    rw_lock->writing = 1;

    pthread_mutex_unlock(&rw_lock->internal_lock);
    return 0;
}

int w_unlock(struct rw_lock_t* rw_lock)
{
    pthread_mutex_lock(&rw_lock->internal_lock);
   
    rw_lock->writing = 0;

    if(rw_lock->writers_waiting > 0)
    {
        pthread_cond_signal(&rw_lock->signal_writers);
    }
    else if(rw_lock->writers_waiting == 0 && rw_lock->readers_waiting > 0)
    {
        pthread_cond_broadcast(&rw_lock->signal_readers);
    }

    pthread_mutex_unlock(&rw_lock->internal_lock);
    return 0;
}
