#include <stdio.h>
#include "alloc.h"

int main(int argc, char* argv[])
{
    #ifdef DEBUG
        printf("-->DEBUG ENABLED\n");
    #endif

    void* foo[10];

    for(int i = 0; i < 10; ++i)
    {
        foo[i] = alloc(i);
    }

    for(int i = 0; i < 10; ++i)
    {
        printf("Main: %p\n", foo[i]);
    }

    for(int i = 1; i < 10; ++i)
    {
        dealloc(foo[i]);
    }

    printf("Main terminated.\n");
    return 0;
}
