#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <sys/time.h>
#include <sys/resource.h>
#include "alloc.h"

//The amount of blocks to initially alloc and dealloc
#define ARRAY_LENGTH    500 
//The amount of strings to be allocated
#define STRING_LENGTH   4000

//Declare some structs that hold various sizes of data which will be allocated
//and deallocated later
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

//Declare them as globals so we dont have issues with overloading the stack
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

    //Parse the args, we only take the stratergy or if no input we set to first
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

    //Random seed
    srand(time(0));

    // Allocate a bunch of structs of varying sizes randomly
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

    //Dealloc all the allocations in random order
    for(i = 0; i < ARRAY_LENGTH; ++i)
    {
        rnum = rand() % 5;
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

    // Now that we have a bunch of random blocks in our freed list
    // we can start allocating random strings and test the performace
    // of the allocation algorithms
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

    // We wrap the allocations in a timer
    gettimeofday(&start, NULL);
    for(int j = 0; j < STRING_LENGTH; ++j)
    {   
        string_alloc[j] = (char*) alloc(getline(&line, &len, names));
    }
    gettimeofday(&end, NULL);
    list();
    printf("Time to allocate: %.3fms\n", (double) (end.tv_sec - start.tv_sec)*1000
            + (double) (end.tv_usec - start.tv_usec)/1000);

    fclose(names);
    printf("Main terminated.\n");
    return 0;
}
