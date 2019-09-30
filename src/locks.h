/*
 * Definitions of locking mechanisms for thread safety
 *
 * Matthew Atkin
 * s3603797
 *
 * Sept 2019
 */
#include <pthread.h>

/* 
 * This macro can be used to statically initilaise the rw_lock, otherwise
 * the init function should be used.
 */
#define RW_LOCK_INIT \
    {PTHREAD_MUTEX_INITIALIZER, PTHREAD_COND_INITIALIZER, \
    PTHREAD_COND_INITIALIZER, 0, 0, 0, 0}

/* 
 * Structure of our rw_lock
 *
 * We use an internal mutex lock to only let one thread at a time access and
 * alter the internals of the structure. We then keep track of how many
 * writers and readers we have waiting and active. We use condvars to wake up
 * threads that we have previously put to sleep. 
 */
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

/*
 * We can use this function to initialise the rw_lock to its default values
 */
int rw_lock_init(struct rw_lock_t* rw_lock);

/*
 * This function destorys the lock, which releases any threads that are being
 * blocked or waiting for anything to do with the lock.
 */
int rw_lock_destroy(struct rw_lock_t* rw_lock);

/*
 * Here we attempt to grab the lock for reading, if there are no writers
 * active or waiting, we increment the readers var and obtain the lock.
 * If there is writers active or waiting, we increment the readers_waiting
 * var and wait until the writers have all been serviced.
 */
int r_lock(struct rw_lock_t* rw_lock);

/*
 * Once we have finished reading, we call this function to release the lock.
 * This simply decrements the readers var and if there are no others current
 * readers and at least 1 writer waiting, we signal a writer to wake up.
 */
int r_unlock(struct rw_lock_t* rw_lock);

/*
 * A writer can only aquire the lock if there are no current readers or
 * writers. If there is, we increment the writers_waiting var and wait
 * until all the readers and/or other writers are done.
 */
int w_lock(struct rw_lock_t* rw_lock);

/*
 * Once all the writing is done, we unlock and do one of two things. If there
 * is more writers waiting, we signal the next one to wake up. If there are no
 * more writers waiting, we simply wake up all the waiting readers and unlock.
 */
int w_unlock(struct rw_lock_t* rw_lock);
