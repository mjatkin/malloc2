#include <stddef.h>
#include <stdio.h>
#include "alloc.h"
#include "block.h"

void* alloc(size_t chunk_size)
{
    sbrk(chunk_size); 
    
    return NULL;
}

void dealloc(void* chunk)
{
    
}
