#include<pthread.h>
#include "alloc.h"
#include <stdio.h>

#define ALLOCS_PER_THREAD 2
#define NO_OF_THREADS 10

void *thread_func(void *unused)
{
    void *alloc_ptrs[ALLOCS_PER_THREAD];

    printf("Thread %ld starting.\n", pthread_self());
    for(int i = 0; i < ALLOCS_PER_THREAD; ++i)
    {
        alloc_ptrs[i] = alloc((i+1)*27);
        printf("Thread %ld: alloc - %p\n", pthread_self(), alloc_ptrs[i]);
    }
    printf("Thread %ld deallocing.\n", pthread_self());
    for(int i = 0; i < ALLOCS_PER_THREAD; ++i)
    {
        dealloc(alloc_ptrs[i]);
        printf("Thread %ld: dealloc - %p\n", pthread_self(), alloc_ptrs[i]);
    }
    printf("Thread %ld allocing.\n", pthread_self());
    for(int i = 0; i < ALLOCS_PER_THREAD; ++i)
    {
        alloc_ptrs[i] = alloc((i+1)*13);
        printf("Thread %ld: alloc - %p\n", pthread_self(), alloc_ptrs[i]);
    }
    return 0;
}

int main(void)
{
    set_stratergy(first);
    pthread_t thr_ids[NO_OF_THREADS];

    for(int i = 0; i < NO_OF_THREADS; ++i)
    {
        if(pthread_create(&thr_ids[i], NULL, thread_func, NULL) != 0)  
        {
            perror("Can not create thread");
        }
    }

    for(int i = 0; i < NO_OF_THREADS; ++i)
    {
        if(pthread_join(thr_ids[i], NULL) != 0)
        {
            perror("Can not join thread");
        }
        printf("Thread %lu joined with main.\n", thr_ids[i]);
    }

    list();

    printf("Main terminated.\n");

    
}
