/*
 * Contains the definition of the lists and memory blocks.
 *
 * Matthew Atkin
 * s3603797
 * Aug 2019
 */
#include <stddef.h>

/*
 * This is the metadata for the allocated memory pointed to by 'data'.
 */
struct block
{
    struct block* next;
    struct block* prev;
    pthread_mutex_t lock;
    size_t size;
    void* data;
} block;

/*
 * Linked list for storing the metadata block structs, with a read write lock
 * that can be utilised to make the list thread safe.
 */
struct linked_list
{
    struct block* head;
    struct block* tail;
    struct rw_lock_t rw_lock;
} linked_list;
