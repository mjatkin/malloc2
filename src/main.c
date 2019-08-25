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

    for(int i = 1; i < 10; ++i)
    {
        dealloc(foo[i]);
    }

    alloc(3);
    printf("\n");
    alloc(3);
    printf("\n");
    alloc(2);
    printf("\n");
    alloc(9);
    printf("\n");
    alloc(15);
    printf("\n");
    printf("Main terminated.\n");
    return 0;
}
