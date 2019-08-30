#include <stdio.h>
#include "alloc.h"

int main(int argc, char* argv[])
{
    #ifdef DEBUG
        printf("-->DEBUG ENABLED\n");
    #endif

    set_stratergy(best);
    void* foo[10];

    for(int i = 0; i < 10; ++i)
    {
        foo[i] = alloc(i);
    }

    for(int i = 1; i < 10; ++i)
    {
        dealloc(foo[i]);
    }

    alloc(3);
    alloc(3);
    alloc(2);
    alloc(9);
    alloc(15);
    alloc(1024);
    alloc(7);
    printf("Main terminated.\n");
    return 0;
}
