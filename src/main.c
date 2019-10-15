#define _GNU_SOURCE
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <sys/time.h>
#include <sys/resource.h>
#include "alloc.h"
#include <pthread.h>

/* The amount of blocks to initially alloc and dealloc */
#define ARRAY_LENGTH    10000

/* No. of threads to utilize to allocate the names */
#define NO_OF_THREADS 1

/* The amount of strings to be allocated */
#define STRING_LENGTH   4000/NO_OF_THREADS

/* Different sizes of our blocks we will intially allocate */
#define TINY_BLOCK_SIZE 8
#define SMALL_BLOCK_SIZE 41
#define MEDIUM_BLOCK_SIZE 128
#define LARGE_BLOCK_SIZE 256
#define HUGE_BLOCK_SIZE 511

/* Amount of types of blocks */
#define BLOCK_TYPE_COUNT 5

/* Milli unit so we can convert times at the end */
#define MILLI 1000

/* Structs that hold various sizes of data which will be allocated and 
 * deallocated initiallly */
struct tiny_block{
    char a[TINY_BLOCK_SIZE];
};

struct small_block{
    char a[SMALL_BLOCK_SIZE];
};

struct medium_block{
    char a[MEDIUM_BLOCK_SIZE];
};

struct large_block{
    char a[LARGE_BLOCK_SIZE];
}; 

struct huge_block{
    char a[HUGE_BLOCK_SIZE];
};

/* Declare these as globals so we dont have issues with overloading the stack */
struct tiny_block* tiny_alloc[ARRAY_LENGTH];
struct small_block* small_alloc[ARRAY_LENGTH];
struct medium_block* medium_alloc[ARRAY_LENGTH];
struct large_block* large_alloc[ARRAY_LENGTH];
struct huge_block* huge_alloc[ARRAY_LENGTH];
int alloc_array[STRING_LENGTH];

/*
 * This is our worker thread function that will be doing the name allocations.
 * It waits for main to deligate some valid data to allocate, and then goes
 * and makes the allocation. Joining back with main once the allocating has
 * been completed.
 */
void *thread_func(void *unused)
{
    for(int j = 0; j < STRING_LENGTH; ++j)
    {   
        alloc(alloc_array[j]);
    }
    return 0;
}

/*
 * Main.
 */
int main(int argc, char* argv[])
{
    #ifdef DEBUG
        printf("-->DEBUG ENABLED\n");
    #endif

    /* Parse the args, we only take the stratergy or if no input we set to
     * first */
    if(argc == 1)
    { 
        set_stratergy(FIRST);
    }
    else if(argc == 2)
    {
        if(strcmp(argv[1], "FIRST") == 0)
        {
            set_stratergy(FIRST);
        }
        else if(strcmp(argv[1], "BEST") == 0)
        {
            set_stratergy(BEST);
        }
        else if(strcmp(argv[1], "WORST") == 0)
        {
            set_stratergy(WORST);
        }
        else
        {
            printf("Error: argument '%s' not valid.\n", argv[1]);
            exit(1);
        }
    }
    else if(argc >= 3)
    {
        printf("Error: Too many arguments\n");
        exit(1);
    }

    /* Random seed */
    srand(time(0));

    /* Allocate a bunch of structs of varying sizes randomly */
    int i, rnum, t_count = 0, s_count = 0, m_count = 0, l_count = 0, h_count = 0;

    for(i = 0; i < ARRAY_LENGTH; ++i)
    {
        rnum = rand() % BLOCK_TYPE_COUNT;
        switch(rnum)
        {
            case 0:
                tiny_alloc[t_count] = (struct tiny_block*) alloc(sizeof(struct tiny_block));
                ++t_count;
                break;
            case 1:
                small_alloc[s_count] = (struct small_block*) alloc(sizeof(struct small_block));
                ++s_count;
                break;
            case 2:
                medium_alloc[m_count] = (struct medium_block*) alloc(sizeof(struct medium_block));
                ++m_count;
                break;
            case 3:
                large_alloc[l_count] = (struct large_block*) alloc(sizeof(struct large_block));
                ++l_count;
                break;
            case 4:
                huge_alloc[h_count] = (struct huge_block*) alloc(sizeof(struct huge_block));
                ++h_count;
                break;
        }
    }

    /* Dealloc all the allocations in random order */
    for(i = 0; i < ARRAY_LENGTH; ++i)
    {
        rnum = rand() % BLOCK_TYPE_COUNT;
        switch(rnum)
        {
            while(1)
            {
                case 0:
                    if(t_count > 0)
                    {
                        --t_count;
                        dealloc(tiny_alloc[t_count]);
                        break;    
                    }
                case 1:
                    if(s_count > 0)
                    {
                        --s_count;
                        dealloc(small_alloc[s_count]);
                        break;    
                    }
                case 2:
                    if(m_count > 0)
                    {
                        --m_count;
                        dealloc(medium_alloc[m_count]);
                        break;    
                    }
                case 3:
                    if(l_count > 0)
                    {
                        --l_count;
                        dealloc(large_alloc[l_count]);
                        break;    
                    }
                case 4:
                    if(h_count > 0)
                    {
                        --h_count;
                        dealloc(huge_alloc[h_count]);
                        break;    
                    }
            }
        }
    }

    /* Now that we have a bunch of random blocks in our freed list
     * we can start allocating random strings and test the performace
     * of the allocation algorithms. */
    FILE* names; 
    char* line = NULL;
    size_t len = 0;
    names = fopen("data/first-names.txt", "r");
    if(names == NULL)
    {
        printf("Can't open file!\n");
        exit(1);
    }

    for(int j = 0; j < STRING_LENGTH; ++j)
    {   
        alloc_array[j] = getline(&line, &len, names);
    }

    struct timeval start, end;
    pthread_t thr_ids[NO_OF_THREADS];
    gettimeofday(&start, NULL);

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
    }

    gettimeofday(&end, NULL);
    list();
    printf("Time to allocate: %.3fms\n", (double) (end.tv_sec - start.tv_sec)*MILLI
            + (double) (end.tv_usec - start.tv_usec)/MILLI);

    fclose(names);
    printf("Main terminated.\n");
    return 0;
}
