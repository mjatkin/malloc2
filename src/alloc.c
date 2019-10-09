/*
 * Implementation of the memory allocator.
 *
 * Matthew Atkin
 * s3603797
 * 
 * Oct 2019
 */
#define _GNU_SOURCE
#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include "alloc.h"
#include "locks.h"
#include "list.h"

/* Stratergy we are currently using for the allocator (defaults to FIRST) */
static enum stratergy current_stratergy = FIRST;

/* Linked lists for the alloc and freed lists */
static struct linked_list alloc_list = {NULL, NULL, RW_LOCK_INIT};
static struct linked_list freed_list = {NULL, NULL, RW_LOCK_INIT};

/* Mutex to apply thread safety to sbrk(), as it is not natively thread safe */
static pthread_mutex_t sbrk_lock = PTHREAD_MUTEX_INITIALIZER;

/*
 * Prints out the current freed and alloc lists and all the data
 * assosiated with them as well as some stats about them.
 */
void list()
{
    int alloc_count = 0, freed_count = 0, alloc_total = 0, freed_total = 0;

    /* Print out the entire alloc_list linked list */
    r_lock(&alloc_list.rw_lock);

    struct block* alloc_current = alloc_list.head;
    printf("\n\nALLOC LIST\n----------\n");
    while(alloc_current != NULL)
    {
        printf("-->Block: %p, Next: %p, Prev: %p, Size: %ld, Data: %p\n", 
            (void*) alloc_current, (void*) alloc_current->next,
            (void*) alloc_current->prev, alloc_current->size, 
            alloc_current->data);
        ++alloc_count;
        alloc_total += alloc_current->size;
        alloc_current = alloc_current->next;
    }
    printf("-->Head: %p\n", (void*) alloc_list.head);
    printf("-->Tail: %p\n", (void*) alloc_list.tail);

    r_unlock(&alloc_list.rw_lock);

    /* Print out the entire freed_list linked list */
    r_lock(&freed_list.rw_lock);

    struct block* freed_current = freed_list.head;
    printf("\n\nFREED LIST\n----------\n");
    while(freed_current != NULL)
    {
        printf("-->Block: %p, Next: %p, Prev: %p, Size: %ld, Data: %p\n", 
            (void*) freed_current, (void*) freed_current->next, 
            (void*) freed_current->prev, freed_current->size, 
            freed_current->data);
        ++freed_count;
        freed_total += freed_current->size;
        freed_current = freed_current->next;
    }
    printf("-->Head: %p\n", (void*) freed_list.head);
    printf("-->Tail: %p\n", (void*) freed_list.tail);

    r_unlock(&freed_list.rw_lock);

    /* Print total nodes and average block sizes of each list */
    printf("Alloc list size: %d\n", alloc_count);
    printf("Freed list size: %d\n", freed_count);
    printf("Alloc average block size: %f\n", (float)alloc_total/alloc_count);
    printf("Freed average block size: %f\n", (float)freed_total/freed_count);
}

/*
 * Simply push the program heap break forward by the passed in size and return
 * the pointer to the data just created.
 * 
 * Mutex locks are utilised here to ensure it is thread safe.
 * 
 * If this fucntion is to fail, we abort the program.
 */
static void* change_break(size_t chunk_size)
{
    /* Mutually this sections so calls to sbrk() are thread safe */
    pthread_mutex_lock(&sbrk_lock);

    #ifdef DEBUG
    printf("-->Moving program break %ld bytes forward. (%p -> %p)\n", 
        chunk_size, sbrk(0), (void*)((char*) sbrk(0) + chunk_size));
    #endif

    void* sbrk_ret = sbrk(chunk_size);

    pthread_mutex_unlock(&sbrk_lock);

    /* If we get (void*) -1 returned from sbrk() then it has failed and we need
     * to abort the program. */
    if(sbrk_ret == (void*) -1)
    {
        perror("'sbrk()' failed unexpectedly");
        abort();
    }
    return sbrk_ret;
}

/*
 * Allocates both the requested size 'chunk_size' and the metadata block
 * assosiated with it on the heap and returns a pointer to the metadata block.
 */
static struct block* create_block(size_t chunk_size)
{
    /* Allocate the metadata block on the heap */
    struct block* current_block = 
        (struct block*) change_break(sizeof(struct block));
    
    /* Initialise some default values */
    current_block->next = NULL;
    current_block->prev = NULL;
    if(pthread_mutex_init(&current_block->lock, NULL))
    {
        perror("'pthread_mutex_init' failed unexpectedly");
        abort();
    }

    /* Allocate the requested data on the heap and set the data ptr to 
     * the allocation, as well as record the size of this allocation */
    current_block->data = change_break(chunk_size);
    current_block->size = chunk_size;

    #ifdef DEBUG
    printf("-->Created block (Block: %p, Next: %p, Prev: %p, Size: %ld,"
        "Data: %p)\n", 
        (void*) current_block, (void*) current_block->next, 
        (void*) current_block->prev, current_block->size, current_block->data);
    #endif

    return current_block;
}

/*
 * Split the block passed in down to the size specified, returning a new 
 * block with the size that is left over.
 */
static struct block* split_block(struct block* block, size_t new_size)
{
    #ifdef DEBUG
    printf("-->Splitting block (Block: %p, Next: %p, Prev: %p, Size: %ld,"
        "Data: %p) down to %ld and %ld\n", 
        (void*) block, (void*) block->next, (void*) block->prev, 
        block->size, block->data, new_size, block->size - new_size);
    #endif

    /* Create a new block with the left over size */
    struct block* new_block = create_block(block->size - new_size);

    /* Set the block we are splitting to its smaller new size */
    block->size = new_size;

    /* Give the new block a pointer to the data of the old block, but offset
     * by the old blocks new size */
    new_block->data = (void *) (((char *) block->data) + new_size);

    return new_block;
}

/*
 * Append a block pointer to the back of alloc_list
 */
static void alloc_list_append(struct block* block)
{
    /* If this is the first ever block we need to set it as the head */
    if(alloc_list.head == NULL)
    {
        alloc_list.head = block;
    }

    /* Set the current tail's next block to this block and this blocks previous
     * to the current tail (if there is a tail) */
    if(alloc_list.tail != NULL)
    {
        alloc_list.tail->next = block;
        block->prev = alloc_list.tail;
    }

    /* This block will always become the new tail */
    alloc_list.tail = block;

    #ifdef DEBUG
    printf("-->Appended block (Block: %p, Next: %p, Prev: %p, Size: %ld,"
        "Data: %p) to back of alloc_list.\n", 
        (void*) block, (void*) block->next, (void*) block->prev,
        block->size, block->data);
    #endif
}

/*
 * Append a block pointer to the back of freed_list
 */
static void freed_list_append(struct block* block)
{
    /* If this is the first ever block we need to set it as the head */
    if(freed_list.head == NULL)
    {
        freed_list.head = block;
    }

    /* Set the current tail's next block to this block and this blocks previous
     * to the current tail (if there is a tail) */
    if(freed_list.tail != NULL)
    {
        freed_list.tail->next = block;
        block->prev = freed_list.tail;
    }

    /* This block will always become the new tail */
    freed_list.tail = block;

    #ifdef DEBUG
    printf("-->Appended block (Block: %p, Next: %p, Prev: %p, Size: %ld,"
        "Data: %p) to back of freed_list.\n", 
        (void*) block, (void*) block->next, (void*) block->prev,
        block->size, block->data);
    #endif
}

/*
 * Delete the specified block from whichever list it is in
 */
static void list_delete(struct block* block)
{
    #ifdef DEBUG
    printf("-->Removing block (Block: %p, Next: %p, Prev: %p, Size: %ld,"
        "Data: %p)\n", 
        (void*) block, (void*) block->next, (void*) block->prev,
        block->size, block->data);
    #endif

    /* If the block is a head of either list we need to set the new head as the
     * next block (the next block could be NULL in this case, which is fine) */
    if(block == alloc_list.head)
    {
        alloc_list.head = block->next;
    } 
    else if(block == freed_list.head)
    {
        freed_list.head = block->next;
    }

    /* If the block is a tail of either list we need to set the new tail as the
     * prev block (the prev block could be NULL in this case, which is fine) */
    if(block == alloc_list.tail)
    {
        alloc_list.tail = block->prev;
    } 
    else if(block == freed_list.tail)
    {
        freed_list.tail = block->prev;
    }

    /* If the block has either a next or prev block, then we rebuild the list
     * by linking the prev node with the next node */
    if(block->next != NULL)
    {
        block->next->prev = block->prev;
    }
    if(block->prev != NULL)
    {
        block->prev->next = block->next;
    }

    /* Remove the references to the prev and next so that it is completely
     * unrelated either list */
    block->next = NULL;
    block->prev = NULL;
}

/* 
 * Allocate the passed in block and split it down to the passed in chunk size
 * if the block is larger. It is assumed that the blocks mutex lock is owned by
 * this thread, as it will be unlocked before exiting this function.
 * 
 * If NULL is passed in as the block, we create a new block of the chunk_size
 * and allocate it.
 */
static void* aquire_block(struct block* block, size_t chunk_size)
{
    void* chunk = NULL; // The ptr to the chunk we are returning at the end

    /* If we have found and locked a valid block, we remove it from the freed
     * list (splitting the block if need be) and add it to the alloc list.
     * We also release the lock on the block, so if it gets deallocated at a 
     * later date, another thread is able to lock it for themselves. 
     * 
     * If the block is NULL, we create a new block and add it to the alloc
     * list.
     * 
     * In both cases, write locks are used on the lists, as we are altering the
     * data and need to maintain thread safety */
    if(block != NULL)
    {
        w_lock(&freed_list.rw_lock);

        if(block->size > chunk_size)
        {
            freed_list_append(split_block(block, chunk_size));
        }
        list_delete(block);

        w_unlock(&freed_list.rw_lock);

        w_lock(&alloc_list.rw_lock);

        alloc_list_append(block);
        pthread_mutex_unlock(&block->lock);
        chunk = alloc_list.tail->data;

        w_unlock(&alloc_list.rw_lock);
    }
    else
    {       
        w_lock(&alloc_list.rw_lock);

        alloc_list_append(create_block(chunk_size));
        chunk = alloc_list.tail->data;

        w_unlock(&alloc_list.rw_lock);
    }

    return chunk;
}

/*
 * Attempt to find a suitable block in the freed list for the size passed
 * into the function using the first algorithm, if no suitable block is 
 * found, we create a new block.
 */
static void* alloc_first(size_t chunk_size)
{
    struct block* current_block = NULL; // Our temporary block pointer
    
    /* Here we lock down the list for reading and attempt to find a
     * valid block */
    r_lock(&freed_list.rw_lock);

    current_block = freed_list.head;
    while(current_block != NULL)
    {
        if(current_block->size >= chunk_size)
        {   
            /* We've found a valid block! Now we attempt to lock the block's
             * mutex. If we are able to then we can keep the block and break
             * out of the loop. If another thread has already locked this block
             * then we simply go back to searching */
            if(pthread_mutex_trylock(&current_block->lock) == 0)
            {        
                break;
            }
        }
        current_block = current_block->next;
    }

    r_unlock(&freed_list.rw_lock);

    return aquire_block(current_block, chunk_size);
}

/*
 * Attempt to find a suitable block in the freed list for the size passed
 * into the function using the best algorithm, if no suitable block is 
 * found, we create a new block
 */
static void* alloc_best(size_t chunk_size)
{
    struct block* current_block = NULL; // Our temporary block pointer
    struct block* best_block = NULL; // The currently best suited block

    /* Here we lock down the list for reading and attempt to find the best
     * fitting block */
    r_lock(&freed_list.rw_lock);

    current_block = freed_list.head;
    while(current_block != NULL)
    {
        if(current_block->size >= chunk_size)
        {   
            /* We found a valid block! There are now 3 things that can happen: 
             *
             * 1. The block is of equal size to the size we are requesting.
             * This means we can terminate early as it is the best possible
             * size we will ever find.
             * 
             * 2. The block is not equal and the first valid block we have
             * found. In this case we attempt to lock it and set it as our
             * best block.
             * 
             * 3. The block is not equal, not our first valid block and 
             * is smaller in size than our previous best. In this case we
             * again attempt to aquire the block lock and then relinquish
             * the lock on the previous best block (so another thread can
             * aquire it). Setting the block as our new best.
             * 
             * In all cases, if the lock is not able to be aquired, we simply
             * ignore the block and go back to searching, as this means another
             * thread is currently wanting to use the block*/
            if(current_block->size == chunk_size)
            {
                if(pthread_mutex_trylock(&current_block->lock) == 0)
                {
                    best_block = current_block;
                    break;
                }
            }
            else if(best_block == NULL)
            {
                if(pthread_mutex_trylock(&current_block->lock) == 0)
                {
                    best_block = current_block;
                }
            }
            else if(current_block->size < best_block->size)
            {
                if(pthread_mutex_trylock(&current_block->lock) == 0)
                {
                    pthread_mutex_unlock(&best_block->lock);
                    best_block = current_block;
                }
            }
        }
        current_block = current_block->next;
    }

    r_unlock(&freed_list.rw_lock);

    return aquire_block(best_block, chunk_size);
}

/*
 * Attempt to find a suitable block in the freed list for the size passed
 * into the function using the worst algorithm, if no suitable block is 
 * found, we create a new block
 */
static void* alloc_worst(size_t chunk_size)
{
    struct block* current_block = NULL; // Our temporary block pointer
    struct block* worst_block = NULL; // The currently best suited block
    int valid = 0; // Flag to determine if the current block is valid

    /* Here we lock down the list for reading and attempt to find the worst
     * fitting block */
    r_lock(&freed_list.rw_lock);

    current_block = freed_list.head;
    while(current_block != NULL)
    {
        if(current_block->size >= chunk_size)
        {   
            /* We found a valid block! There are now 2 things that can happen: 
             *
             * 1. The block is the first valid block we have found. In this
             * case we attempt to lock it and set it as our worst block.
             * 
             * 2. The block is not our first valid block and is larger in size
             * than our previous worst. In this case we again attempt to aquire
             * the block lock and then relinquish the lock on the previous
             * worst block (so another thread can aquire it). Setting the block
             * as our new worst.
             * 
             * In both cases, if the lock is not able to be aquired, we simply
             * ignore the block and go back to searching, as this means another
             * thread is currently wanting to use the block*/
            if(worst_block == NULL)
            {
                if(pthread_mutex_trylock(&current_block->lock) == 0)
                {
                    worst_block = current_block;
                    valid = 1;
                }
            }
            else if(current_block->size > worst_block->size)
            {
                if(pthread_mutex_trylock(&current_block->lock) == 0)
                {
                    pthread_mutex_unlock(&worst_block->lock);
                    worst_block = current_block;
                }
            }
        }
        current_block = current_block->next;
    }

    r_unlock(&freed_list.rw_lock);

    return aquire_block(worst_block, chunk_size);
}

/* 
 * Attempt to allocate the given size using the set algorithm
 */
void* alloc(size_t chunk_size)
{  
    #ifdef DEBUG
    printf("\n\n-->Allocating %ld bytes\n", chunk_size);
    #endif

    /* If we attempt to allocate <= 0 bytes we just return null */
    if((signed long long int)chunk_size <= 0)
    {
        return NULL;
    }

    /* Pass off the allocation to whichever algorithm is selected */
    switch(current_stratergy)
    {
        case FIRST:
            #ifdef DEBUG
            printf("-->Allocating using first fit...\n");
            #endif
            return alloc_first(chunk_size);
        case BEST:
            #ifdef DEBUG
            printf("-->Allocating using best fit...\n");
            #endif
            return alloc_best(chunk_size);
        case WORST:
            #ifdef DEBUG
            printf("-->Allocating using worst fit...\n");
            #endif
            return alloc_worst(chunk_size);
        default:
            #ifdef DEBUG
            printf("-->Reached default case of alloc...\n");
            #endif
            abort();
    }
}

/*
 * Attempt to dealloc the block containing the pointer equal to 'chunk'
 */
void dealloc(void* chunk)
{
    struct block* current_block; // Current block we are looking at
    
    /* If we attempt to dealloc NULL then we just return */
    if(chunk == NULL)
    {
        #ifdef DEBUG
        printf("\n\n-->Attempting to dealloc NULL, did nothing\n");
        #endif

        return;
    }

    #ifdef DEBUG
    printf("\n\n-->Attempting to dealloc block with data %p...\n", chunk);
    #endif

    /* Here we aquire the read lock for the alloc list and go through the list
     * in search of the block we wish to deallocate. */
    r_lock(&alloc_list.rw_lock);

    current_block = alloc_list.head;
    while(current_block != NULL)
    { 
        if(current_block->data == chunk)
        {  
            /* We've found the block! So we break out of the loop */
            break;
        }
        current_block = current_block->next;
    }

    r_unlock(&alloc_list.rw_lock);

    /* If we found the block then we attempt to remove it from the alloc list
     * and add it to the freed list. 
     * 
     * However if we couldnt find it, we need to abort the program */
    if(current_block != NULL)
    {
        #ifdef DEBUG
        r_lock(&alloc_list.rw_lock);
        printf("-->Found the block (Block: %p, Next: %p, Prev: %p, Size: %ld, Data: %p)\n", 
        (void*) current_block, (void*) current_block->next, (void*) current_block->prev, 
        current_block->size, current_block->data);
        r_unlock(&alloc_list.rw_lock);
        #endif

        w_lock(&alloc_list.rw_lock);

        list_delete(current_block);

        w_unlock(&alloc_list.rw_lock);

        w_lock(&freed_list.rw_lock);

        freed_list_append(current_block);

        w_unlock(&freed_list.rw_lock);        
    }
    else
    {
        printf("Attempted to deallocate an invalid pointer: %p\n", chunk);
        abort();
    }
}

/*
 * Set the stratergy to be used in allocation
 */
void set_stratergy(enum stratergy stratergy)
{
    current_stratergy = stratergy;
    
    #ifdef DEBUG
    printf("-->Stratergy changed: %d\n", current_stratergy);
    #endif
}
