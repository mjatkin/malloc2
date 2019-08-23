#include <stdio.h>
#include "alloc.h"

int main(int argc, char* argv[])
{
    alloc(4);
    printf("Main terminated.\n");
    return 0;
}
