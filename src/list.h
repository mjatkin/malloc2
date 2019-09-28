/*
 * Contains the definition of the lists and memory blocks.
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

struct linked_list
{
    struct block* head;
    struct block* tail;
    struct rw_lock_t rw_lock;
} linked_list;
