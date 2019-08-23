#include <stddef.h>

struct block
{
    struct block* next;
    struct block* prev;
    size_t size;
    void* data;
} block;
