/*
 * Contains the definition of a memory block.
 *
 * Matthew Atkin
 * s3603797
 * Aug 2019
 */
#include <stddef.h>

struct block
{
    struct block* next;
    struct block* prev;
    size_t size;
    void* data;
} block;
