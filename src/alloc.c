/*
 * Implementation of the memory allocator.
 *
 * Matthew Atkin
 * s3603797
 * 
 * Aug 2019
 */
#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include <unistd.h>
#include "alloc.h"
#include "locks.h"
#include "list.h"

// Stratergy we are currently using for the allocator (default to first)
static enum stratergy current_stratergy = first;

// Alloc and freed lists
static struct linked_list alloc_list = {NULL, NULL, RW_LOCK_INIT};
static struct linked_list freed_list = {NULL, NULL, RW_LOCK_INIT};

/*
 * Prints out the current freed and alloc lists and all the data
 * assosiated with them as well as some stats about them
 */
void list()
{
    int alloc_count = 0, freed_count = 0, alloc_total = 0, freed_total = 0;

    r_lock(&alloc_list.rw_lock);
    struct block* alloc_current = alloc_list.head;
    printf("\n\nALLOC LIST\n----------\n");
    while(alloc_current != NULL)
    {
        printf("-->Block: %p, Next: %p, Location: %d, Prev: %p, Size: %ld, Data: %p\n", 
            (void*) alloc_current, (void*) alloc_current->next, alloc_current->location,
            (void*) alloc_current->prev, alloc_current->size, alloc_current->data);
        ++alloc_count;
        alloc_total += alloc_current->size;
        alloc_current = alloc_current->next;
    }
    printf("-->Head: %p\n", (void*) alloc_list.head);
    printf("-->Tail: %p\n", (void*) alloc_list.tail);
    r_unlock(&alloc_list.rw_lock);

    r_lock(&freed_list.rw_lock);
    struct block* freed_current = freed_list.head;
    printf("\n\nFREED LIST\n----------\n");
    while(freed_current != NULL)
    {
        printf("-->Block: %p, Next: %p, Location: %d, Prev: %p, Size: %ld, Data: %p\n", 
            (void*) freed_current, (void*) freed_current->next, freed_current->location, 
            (void*) freed_current->prev, freed_current->size, freed_current->data);
        ++freed_count;
        freed_total += freed_current->size;
        freed_current = freed_current->next;
    }
    printf("-->Head: %p\n", (void*) freed_list.head);
    printf("-->Tail: %p\n", (void*) freed_list.tail);
    r_unlock(&freed_list.rw_lock);

    // Here we print out the sizes of the lists as well as the average block size
    printf("Alloc list size: %d\n", alloc_count);
    printf("Freed list size: %d\n", freed_count);
    printf("Alloc average block size: %f\n", (float)alloc_total/alloc_count);
    printf("Freed average block size: %f\n", (float)freed_total/freed_count);
}

/*
 * Simply push the program break forward by the passed in size and return
 * the pointer to the data just created
 */
static void* change_break(size_t chunk_size)
{
    #ifdef DEBUG
    printf("-->Moving program break %ld bytes forward. (%p -> %p)\n", 
            chunk_size, sbrk(0), ((char*) sbrk(0)) + chunk_size);
    #endif
    return sbrk(chunk_size);
}

/*
 * Create a new block given a specified size and return a pointer to the block
 */
static struct block* create_block(size_t chunk_size)
{
    struct block* current_block = 
        (struct block*) change_break(sizeof(struct block));
    
    current_block->next = NULL;
    current_block->prev = NULL;
    current_block->location = VOID;
    current_block->size = chunk_size;
    current_block->data = change_break(chunk_size);

    #ifdef DEBUG
    printf("-->Created block (Block: %p, Next: %p, Prev: %p, Size: %ld, Data: %p)\n", 
            (void*) current_block, (void*) current_block->next, 
            (void*) current_block->prev, current_block->size, current_block->data);
    #endif
    return current_block;
}

/*
 * Split the block passed in down to the size specified, returning a new 
 * block with the size that is left over
 */
static struct block* split_block(struct block* block, size_t new_size)
{
    #ifdef DEBUG
    printf("-->Splitting block (Block: %p, Next: %p, Prev: %p, Size: %ld, Data: %p) down to %ld and %ld\n", 
            (void*) block, (void*) block->next, 
            (void*) block->prev, block->size, block->data, new_size, block->size - new_size);
    #endif

    // Create a new block with the left over size
    struct block* new_block = create_block(block->size - new_size);

    // Set the block we are splitting to its smaller new size
    block->size = new_size;

    // Give the new block a pointer to the data of the old block, but offset
    // by the new size.
    new_block->data = (void *) (((char *) block->data) + new_size);

    return new_block;
}

/*
 * Append a block pointer to the back of the alloc list
 */
static void alloc_list_append(struct block* block)
{
    if(alloc_list.head == NULL)
    {
        alloc_list.head = block;
    }
    if(alloc_list.tail != NULL)
    {
        alloc_list.tail->next = block;
        block->prev = alloc_list.tail;
    }
    alloc_list.tail = block;

    block->location = ALLOCD;

    #ifdef DEBUG
    printf("-->Appended block (Block: %p, Next: %p, Prev: %p, Size: %ld, Data: %p) to"
            " back of alloc_list.\n", 
            (void*) block, (void*) block->next, (void*) block->prev,
            block->size, block->data);
    #endif
}

/*
 * Append a block pointer to the back of the freed list
 */
static void freed_list_append(struct block* block)
{
    if(freed_list.head == NULL)
    {
        freed_list.head = block;
    }
    if(freed_list.tail != NULL)
    {
        freed_list.tail->next = block;
        block->prev = freed_list.tail;
    }
    freed_list.tail = block;

    block->location = FREED;

    #ifdef DEBUG
    printf("-->Appended block (Block: %p, Next: %p, Prev: %p, Size: %ld, Data: %p) to"
            " back of freed_list.\n", 
            (void*) block, (void*) block->next, (void*) block->prev, block->size, block->data);
    #endif
}

/*
 * Delete the specified block from whichever list it is in
 */
static void list_delete(struct block* block)
{
    #ifdef DEBUG
    printf("-->Removing block (Block: %p, Next: %p, Prev: %p, Size: %ld, Data: %p)\n", 
            (void*) block, (void*) block->next, (void*) block->prev, block->size, block->data);
    #endif
    if(block == alloc_list.head)
    {
        alloc_list.head = block->next;
    } 
    else if(block == freed_list.head)
    {
        freed_list.head = block->next;
    }

    if(block == alloc_list.tail)
    {
        alloc_list.tail = block->prev;
    } 
    else if(block == freed_list.tail)
    {
        freed_list.tail = block->prev;
    }

    if(block->next != NULL)
    {
        block->next->prev = block->prev;
    }
    if(block->prev != NULL)
    {
        block->prev->next = block->next;
    }
    block->next = NULL;
    block->prev = NULL;
    block->location = VOID;
}

/*
 * Attempt to find a suitable block in the freed list for the size passed
 * into the function using the first algorithm, if no suitable block is 
 * found, we create a new block
 */
static void* alloc_first(size_t chunk_size)
{
    struct block* current_block; // Our temporary block pointer
    void* chunk; // The ptr to the chunk we are returning at the end
    int split = 0; // Flag to determine if we need to split the found block
    int valid = 0; // Flag to determine if the current block is valid
    
    /* We are essentially stuck in this loop until we either find no valid
     * block or we find a valid block and then write to it before another
     * thread. */
    while(1)
    {
        /* Here we lock down the list for reading and attempt to find a
         * valid block */
        r_lock(&freed_list.rw_lock);
        current_block = freed_list.head;
        while(current_block != NULL)
        {
            if(current_block->size >= chunk_size)
            {   
                /* We've found a valid block so we need to check if it needs
                 * to be split or if it is already the correct size then
                 * simply set the valid flag and break out of the loop. */
                if(current_block->size > chunk_size)
                {
                    split = 1;
                }
                valid = 1;
                break;
            }
            current_block = current_block->next;
        }
        r_unlock(&freed_list.rw_lock);

        /* If the block we found was valid then we attempt to allocate it, but
         * if no valid block was found then we just append a new block to the
         * back of the alloc list. */
        if(valid)
        {
            /* Here we lock down the freed list and then attempt to remove
             * the block we which to allocate. We need to be wary that another
             * thread may have already altered this block inbetween when we
             * read it and now. */
            w_lock(&freed_list.rw_lock);
            if(current_block->location == FREED)
            {
                /* If we get in here then the block is still valid, no
                 * other thread has written to it inbetween us reading it
                 * and now. */
                if(split)
                {
                    freed_list_append(split_block(current_block, chunk_size));
                }
                list_delete(current_block);
            }
            else
            {
                /* If we get down here then the block has been written to
                 * since the last time we read it and might not be valid
                 * anymore. So we mark as invalid and start the search
                 * again. */
                valid = 0;
            }
            w_unlock(&freed_list.rw_lock);

            /* We only do this if our block was actually valid.
             * We simply lock down the alloc list and append our block to
             * the end. Returning the valid data ptr afterwards. */
            if(valid)
            {
                w_lock(&alloc_list.rw_lock);
                alloc_list_append(current_block);
                chunk = alloc_list.tail->data;
                w_unlock(&alloc_list.rw_lock);
                return chunk;
            }
        }
        else
        {
            #ifdef DEBUG
            printf("-->No valid block found...\n");
            #endif
            
            /* Simply lock the alloc list down and create a new block to be
             * appended to the end of the list. Returning the ptr to the new
             * block. */
            w_lock(&alloc_list.rw_lock);
            alloc_list_append(create_block(chunk_size));
            chunk = alloc_list.tail->data;
            w_unlock(&alloc_list.rw_lock);
            return chunk;
        }
    }
}

/*
 * Attempt to find a suitable block in the freed list for the size passed
 * into the function using the best algorithm, if no suitable block is 
 * found, we create a new block
 */
static void* alloc_best(size_t chunk_size)
{
    struct block* current_block = freed_list.head;
    struct block* best_block = NULL;

    // Iterate through the freed list
    while(current_block != NULL)
    {
        if(current_block->size >= chunk_size)
        {   
            // We found a valid block
            if(current_block->size == chunk_size)
            {
                // The block is as good as we can get so we allocate it
                // and terminate early
                list_delete(current_block);
                alloc_list_append(current_block);
                return alloc_list.tail->data;
            }
            else if(best_block == NULL)
            {
                // This block is the first valid and is not the exact size
                // so we assign it straight up. This is to stop a possible null
                // dereference in the following else if statement
                best_block = current_block;
            }
            else if(current_block->size < best_block->size)
            {
                // The block is less than our best so we set it as the new best
                best_block = current_block;
            }
        }
        current_block = current_block->next;
    }

    if(best_block == NULL)
    {
        #ifdef DEBUG
        printf("-->No valid block found...\n");
        #endif

        // We didnt find any valid blocks so we make a new one, allocate it
        // and return the data
        alloc_list_append(create_block(chunk_size));
        return alloc_list.tail->data;
    }
    // If we make it here then we found a valid block that wasnt equal in size
    // so we have to split it and allocate it, then return the data to the user
    freed_list_append(split_block(best_block, chunk_size));
    list_delete(best_block);
    alloc_list_append(best_block);
    return alloc_list.tail->data;
}

/*
 * Attempt to find a suitable block in the freed list for the size passed
 * into the function using the worst algorithm, if no suitable block is 
 * found, we create a new block
 */
static void* alloc_worst(size_t chunk_size)
{
    struct block* current_block = freed_list.head;
    struct block* worst_block = NULL;

    // Iterate through the freed list
    while(current_block != NULL)
    {
        if(current_block->size >= chunk_size)
        {   
            // We found a valid block
            if(worst_block == NULL)
            {
                // This block is the first valid. This is to stop a possible
                // null dereference in the following else if statement
                worst_block = current_block;
            }
            else if(current_block->size > worst_block->size)
            {
                // The block is bigger than our current worst so we set it as
                // the new worst
                worst_block = current_block;
            }
        }
        current_block = current_block->next;
    }
    if(worst_block == NULL)
    {
        #ifdef DEBUG
        printf("-->No valid block found...\n");
        #endif

        // We found no valid block so we create a new one, allocate it
        // and return the data
        alloc_list_append(create_block(chunk_size));
        return alloc_list.tail->data;
    }
    else if(worst_block->size != chunk_size)
    {
        // The block we found isnt equal so we need to split it
        freed_list_append(split_block(worst_block, chunk_size));
    }

    // If we make it here we have a valid block, so we allocate it and return
    // the data 
    list_delete(worst_block);
    alloc_list_append(worst_block);
    return alloc_list.tail->data;
}

/* 
 * Attempt to allocate the given size using the set algorithm
 */
void* alloc(size_t chunk_size)
{
    #ifdef DEBUG
    list();
    #endif
    if((signed long long int)chunk_size <= 0)
    {
        #ifdef DEBUG
        printf("\n\n-->Attempted to allocate 0 or negative bytes\n");
        #endif

        // If we get in here we are trying to allocate 0 or negative memory
        // so we just return null
        return NULL;
    }
    
    #ifdef DEBUG
    printf("\n\n-->Allocating %ld bytes\n", chunk_size);
    #endif

    // Pass off the allocation to whichever alg is set if the free
    // list needs to be traversed
    switch(current_stratergy)
    {
        case first:
            #ifdef DEBUG
            printf("-->Allocating using first fit...\n");
            #endif
            return alloc_first(chunk_size);
        case best:
            #ifdef DEBUG
            printf("-->Allocating using best fit...\n");
            #endif
            return alloc_best(chunk_size);
        case worst:
            #ifdef DEBUG
            printf("-->Allocating using worst fit...\n");
            #endif
            return alloc_worst(chunk_size);
        default:
            #ifdef DEBUG
            printf("-->Reached default case of alloc, something isn't right\n");
            #endif
            return NULL;
    }
}

/*
 * Attempt to dealloc the block containing the pointer passed in
 */
void dealloc(void* chunk)
{
    #ifdef DEBUG
    list();
    #endif
    if(chunk == NULL)
    {
        #ifdef DEBUG
        printf("\n\n-->Attempting to dealloc NULL, did nothing\n");
        #endif

        // Deallocing NULL does nothing
        return;
    }

    #ifdef DEBUG
    printf("\n\n-->Attempting to dealloc block with data %p...\n", chunk);
    #endif

    struct block* current_block;
    int valid = 0;

    r_lock(&alloc_list.rw_lock);
    current_block = alloc_list.head;
    
    // Iterate through alloc list
    while(current_block != NULL)
    { 
        if(current_block->data == chunk)
        {  
            valid = 1;
            break;
        }
        current_block = current_block->next;
    }
    r_unlock(&alloc_list.rw_lock);

    if(valid)
    {
        #ifdef DEBUG
        r_lock(&alloc_list.rw_lock);
        printf("-->Found the block (Block: %p, Next: %p, Prev: %p, Size: %ld, Data: %p)\n", 
        (void*) current_block, (void*) current_block->next, (void*) current_block->prev, 
        current_block->size, current_block->data);
        r_unlock(&alloc_list.rw_lock);
        #endif

        // Make sure another thread hasn't already dealloced it
        if(current_block->location == ALLOCD)
        {
            w_lock(&alloc_list.rw_lock);
            list_delete(current_block);
            w_unlock(&alloc_list.rw_lock);

            w_lock(&freed_list.rw_lock);
            freed_list_append(current_block);
            w_unlock(&freed_list.rw_lock);
        }
    }
    else
    {
        #ifdef DEBUG
        printf("-->No valid block found...\n");
        #endif
        // If we dont find the block we exit the program
        exit(1);
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

