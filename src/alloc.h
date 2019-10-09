/*
 * Header file for alloc.c - The programmer needs only use the alloc and
 * dealloc functions when allocating memory on the heap. The programmer
 * may also use the set_stratergy function to set the allocation stratergy.
 *
 * Matthew Atkin
 * s3603797
 * Aug 2019
 */
#include <stddef.h>

/*
 * Searching stratergy's used when allocating memory.
 *
 * first - Finds the first chunk that is large enough for the required memory
 *         and adds any leftover memory to the free list.
 * best  - Finds the chunk that is the closest in size to the required memory
 *         and adds any remaining memory to the free list.
 * worst - Finds the largest chunk and adds the remaining memory to the free
 *         list.
 */
enum stratergy{FIRST, BEST, WORST};

/*
 * Sets the search stratergy for the memory allocator.
 */
void set_stratergy(enum stratergy stratergy);

/*
 * Prints out the current free and alloc lists
 */
void list();

/*
 * Given the passed in size of memory that needs to be allocated, the free list
 * is traversed to see if it can fit it anywhere. If there is a chunk that can
 * hold the required data, it is added to the allocated list. If there is no
 * valid chunk found, then the memory is aquired using sbrk and added to the
 * allocation list.
 */
void* alloc(size_t chunk_size);

/*
 * The allocated list is traversed to find the chunk that holds the pointer to
 * the data that needs to be free'd. If found, the chunk is moved to the free
 * list. If the chunk doesn't exist, the program terminates.
 */
void dealloc(void* chunk);

