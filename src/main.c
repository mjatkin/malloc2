#include<pthread.h>
#include "alloc.h"
#include <stdio.h>

#define ALLOCS_PER_THREAD 20
#define NO_OF_THREADS 100

void *thread_func(void *unused)
{
    void *alloc_ptrs[ALLOCS_PER_THREAD];

    printf("Thread %ld starting.\n", pthread_self());
    for(int i = 0; i < ALLOCS_PER_THREAD; ++i)
    {
        alloc_ptrs[i] = alloc((i+1)*5);
        //printf("Thread %ld: alloc - %p\n", pthread_self(), alloc_ptrs[i]);
    }
    printf("Thread %ld deallocing.\n", pthread_self());
    for(int i = 0; i < ALLOCS_PER_THREAD; ++i)
    {
        dealloc(alloc_ptrs[i]);
        //printf("Thread %ld: dealloc - %p\n", pthread_self(), alloc_ptrs[i]);
    }
    printf("Thread %ld allocing.\n", pthread_self());
    for(int i = 0; i < ALLOCS_PER_THREAD; ++i)
    {
        alloc_ptrs[i] = alloc((i+1)*3);
        //printf("Thread %ld: alloc - %p\n", pthread_self(), alloc_ptrs[i]);
    }
    return 0;
}

int main(void)
{
    set_stratergy(worst);
    pthread_t thr_ids[NO_OF_THREADS];
/*
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

*/
    void *test1 = alloc(10);
    void *test2 = alloc(8);
    void *test3 = alloc(6);
    void *test4 = alloc(9);
    void *test5 = alloc(7);
    dealloc(test1);
    dealloc(test2);
    dealloc(test3);
    dealloc(test4);
    dealloc(test5);
    alloc(5);


    list();

    printf("Main terminated.\n");

    
}
