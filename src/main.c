#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <sys/time.h>
#include <sys/resource.h>
#include "alloc.h"

#define ARRAY_LENGTH 100
#define STRING_LENGTH 4000

struct tiny_block{
    char a[8];
};

struct small_block{
    char a[41];
};

struct medium_block{
    char a[128];
};

struct large_block{
    char a[256];
}; 

struct huge_block{
    char a[511];
};

struct tiny_block* tiny_alloc[ARRAY_LENGTH];
struct small_block* small_alloc[ARRAY_LENGTH];
struct medium_block* medium_alloc[ARRAY_LENGTH];
struct large_block* large_alloc[ARRAY_LENGTH];
struct huge_block* huge_alloc[ARRAY_LENGTH];
char* string_alloc[STRING_LENGTH];

int main(int argc, char* argv[])
{
    #ifdef DEBUG
        printf("-->DEBUG ENABLED\n");
    #endif

    if(argc == 1)
    { 
        set_stratergy(first);
    }
    else if(argc == 2)
    {
        if(strcmp(argv[1], "first") == 0)
        {
            set_stratergy(first);
        }
        else if(strcmp(argv[1], "best") == 0)
        {
            set_stratergy(best);
        }
        else if(strcmp(argv[1], "worst") == 0)
        {
            set_stratergy(worst);
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

    srand(time(0));

    int i, rnum, t_count = 0, s_count = 0, m_count = 0, l_count = 0, h_count = 0;

    for(i = 0; i < ARRAY_LENGTH; ++i)
    {
        rnum = rand() % 5;
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

    for(i = 0; i < ARRAY_LENGTH; ++i)
    {
        rnum = rand() % 5;
        switch(rnum)
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

    FILE* names; 
    names = fopen("data/first-names.txt", "r");
    if(names == NULL)
    {
        printf("Can't open file!\n");
        exit(1);
    }
    struct timeval start, end;
    char* line = NULL;
    size_t len = 0;

    gettimeofday(&start, NULL);
    for(int j = 0; j < STRING_LENGTH; ++j)
    {   
        string_alloc[j] = (char*) alloc(getline(&line, &len, names));
    }
    gettimeofday(&end, NULL);
    list();
    printf("Time to allocate: %.3fms\n", (double) (end.tv_sec - start.tv_sec)*1000 + (double) (end.tv_usec - start.tv_usec)/1000);

    fclose(names);


    /*
    struct rusage usage;
    getrusage(RUSAGE_SELF, &usage);
    printf("ru_utime:    %ld\n"
           "ru_stime:    %ld\n"
           "ru_maxrss:   %ld\n"
           "ru_ixrss:    %ld\n"
           "ru_idrss:    %ld\n"
           "ru_minflt:   %ld\n"
           "ru_majflt:   %ld\n"
           "ru_nswap:    %ld\n"
           "ru_inblock:  %ld\n"
           "ru_oublock:  %ld\n"
           "ru_msgsnd:   %ld\n"
           "ru_msgrcv:   %ld\n"
           "ru_nsignals: %ld\n"
           "ru_nvcsw:    %ld\n"
           "ru_nivcsw:   %ld\n"
           , usage.ru_utime.tv_usec, usage.ru_stime.tv_usec
           , usage.ru_maxrss, usage.ru_ixrss, usage.ru_idrss
           , usage.ru_minflt, usage.ru_majflt, usage.ru_nswap
           , usage.ru_inblock, usage.ru_oublock, usage.ru_msgsnd
           , usage.ru_msgrcv, usage.ru_nsignals, usage.ru_nvcsw
           , usage.ru_nivcsw);
    */
    printf("Main terminated.\n");
    return 0;
    }
