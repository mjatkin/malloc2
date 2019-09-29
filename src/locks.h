/*
 * Definitions of locking mechanisms for thread safety
 *
 * Matthew Atkin
 * s3603797
 *
 * Sept 2019
 */
#include <pthread.h>

#define RW_LOCK_INIT \
    {PTHREAD_MUTEX_INITIALIZER, PTHREAD_COND_INITIALIZER, \
    PTHREAD_COND_INITIALIZER, 0, 0, 0, 0}

struct rw_lock_t
{
    pthread_mutex_t internal_lock;
    pthread_cond_t signal_readers;
    pthread_cond_t signal_writers;
    unsigned int readers;
    unsigned int writing;
    unsigned int readers_waiting;
    unsigned int writers_waiting;
} rw_lock_t;

int rw_lock_init(struct rw_lock_t* rw_lock);

int rw_lock_destroy(struct rw_lock_t* rw_lock);

int r_lock(struct rw_lock_t* rw_lock);

int r_unlock(struct rw_lock_t* rw_lock);

int w_lock(struct rw_lock_t* rw_lock);

int w_unlock(struct rw_lock_t* rw_lock);
