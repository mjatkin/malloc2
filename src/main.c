#include <stdio.h>
#include "alloc.h"

int main(int argc, char* argv[])
{
    #ifdef DEBUG
        printf("-->DEBUG ENABLED\n");
    #endif

    alloc(3);

    printf("Main terminated.\n");
    return 0;
}
