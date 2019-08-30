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
#include "block.h"

static enum stratergy current_stratergy = first;

static struct block* alloc_list_head = NULL;
static struct block* alloc_list_tail = NULL;
static struct block* freed_list_head = NULL;
static struct block* freed_list_tail = NULL;

#ifdef DEBUG
static void list()
{
    struct block* alloc_current = alloc_list_head;
    struct block* freed_current = freed_list_head;
    
    printf("\n\nALLOC LIST\n----------\n");
    while(alloc_current != NULL)
    {
        printf("-->Block: %p, Next: %p, Prev: %p, Size: %ld, Data: %p\n", 
            (void*) alloc_current, (void*) alloc_current->next, 
            (void*) alloc_current->prev, alloc_current->size, alloc_current->data);
        alloc_current = alloc_current->next;
    }
    printf("-->Head: %p\n", (void*) alloc_list_head);
    printf("-->Tail: %p\n", (void*) alloc_list_tail);

    printf("\n\nFREED LIST\n----------\n");
    while(freed_current != NULL)
    {
        printf("-->Block: %p, Next: %p, Prev: %p, Size: %ld, Data: %p\n", 
            (void*) freed_current, (void*) freed_current->next, 
            (void*) freed_current->prev, freed_current->size, freed_current->data);
        freed_current = freed_current->next;
    }
    printf("-->Head: %p\n", (void*) freed_list_head);
    printf("-->Tail: %p\n", (void*) freed_list_tail);
}
#endif

static void* change_break(size_t chunk_size)
{
    #ifdef DEBUG
    printf("-->Moving program break %ld bytes forward. (%p -> %p)\n", 
            chunk_size, sbrk(0), ((char*) sbrk(0)) + chunk_size);
    #endif
    return sbrk(chunk_size);
}

static struct block* create_block(size_t chunk_size)
{
    struct block* current_block = 
        (struct block*) change_break(sizeof(struct block));
    
    current_block->next = NULL;
    current_block->prev = NULL;
    current_block->size = chunk_size;
    current_block->data = change_break(chunk_size);

    #ifdef DEBUG
    printf("-->Created block (Block: %p, Next: %p, Prev: %p, Size: %ld, Data: %p)\n", 
            (void*) current_block, (void*) current_block->next, 
            (void*) current_block->prev, current_block->size, current_block->data);
    #endif
    return current_block;
}


static struct block* split_block(struct block* block, size_t new_size)
{
    #ifdef DEBUG
    printf("-->Splitting block (Block: %p, Next: %p, Prev: %p, Size: %ld, Data: %p) down to %ld and %ld\n", 
            (void*) block, (void*) block->next, 
            (void*) block->prev, block->size, block->data, new_size, block->size - new_size);
    #endif
    struct block* new_block = create_block(block->size - new_size);

    block->size = new_size;
    new_block->data = (void *) (((char *) block->data) + new_size);

    return new_block;
}

static void alloc_list_append(struct block* block)
{
    if(alloc_list_head == NULL)
    {
        alloc_list_head = block;
    }
    if(alloc_list_tail != NULL)
    {
        alloc_list_tail->next = block;
        block->prev = alloc_list_tail;
    }
    alloc_list_tail = block;

    #ifdef DEBUG
    printf("-->Appended block (Block: %p, Next: %p, Prev: %p, Size: %ld, Data: %p) to"
            " back of alloc_list.\n", 
            (void*) block, (void*) block->next, (void*) block->prev,
            block->size, block->data);
    #endif
}

static void freed_list_append(struct block* block)
{
    if(freed_list_head == NULL)
    {
        freed_list_head = block;
    }
    if(freed_list_tail != NULL)
    {
        freed_list_tail->next = block;
        block->prev = freed_list_tail;
    }
    freed_list_tail = block;

    #ifdef DEBUG
    printf("-->Appended block (Block: %p, Next: %p, Prev: %p, Size: %ld, Data: %p) to"
            " back of freed_list.\n", 
            (void*) block, (void*) block->next, (void*) block->prev, block->size, block->data);
    #endif
}

static void list_delete(struct block* block)
{
    #ifdef DEBUG
    printf("-->Removing block (Block: %p, Next: %p, Prev: %p, Size: %ld, Data: %p)\n", 
            (void*) block, (void*) block->next, (void*) block->prev, block->size, block->data);
    #endif
    if(block == alloc_list_head)
    {
        alloc_list_head = block->next;
    } 
    else if(block == freed_list_head)
    {
        freed_list_head = block->next;
    }

    if(block == alloc_list_tail)
    {
        alloc_list_tail = block->prev;
    } 
    else if(block == freed_list_tail)
    {
        freed_list_tail = block->prev;
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
}

static void* alloc_first(size_t chunk_size)
{
    struct block* current_block = freed_list_head;

    while(current_block != NULL)
    {
        if(current_block->size >= chunk_size)
        {   
            if(current_block->size > chunk_size)
            {
                freed_list_append(split_block(current_block, chunk_size));
            }
            list_delete(current_block);
            alloc_list_append(current_block);
            return current_block->data;
        }
        current_block = current_block->next;
    }

    #ifdef DEBUG
    printf("-->No valid block found...\n");
    #endif
    alloc_list_append(create_block(chunk_size));
    return alloc_list_tail->data;
}

static void* alloc_best(size_t chunk_size)
{
    struct block* current_block = freed_list_head;
    struct block* best_block = NULL;

    while(current_block != NULL)
    {
        if(current_block->size >= chunk_size)
        {   
            if(best_block == NULL)
            {
                best_block = current_block;
            }
            else if(current_block->size < best_block->size)
            {
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
        alloc_list_append(create_block(chunk_size));
        return alloc_list_tail->data;
    }
    else if(best_block->size > chunk_size)
    {
        freed_list_append(split_block(best_block, chunk_size));
    }
    list_delete(best_block);
    alloc_list_append(best_block);
    return alloc_list_tail->data;
}

static void* alloc_worst(size_t chunk_size)
{
    struct block* current_block = freed_list_head;
    struct block* worst_block = NULL;

    while(current_block != NULL)
    {
        if(current_block->size >= chunk_size)
        {   
            if(worst_block == NULL)
            {
                worst_block = current_block;
            }
            else if(current_block->size > worst_block->size)
            {
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
        alloc_list_append(create_block(chunk_size));
        return alloc_list_tail->data;
    }
    else if(worst_block->size > chunk_size)
    {
        freed_list_append(split_block(worst_block, chunk_size));
    }
    list_delete(worst_block);
    alloc_list_append(worst_block);
    return alloc_list_tail->data;
}

void* alloc(size_t chunk_size)
{
    #ifdef DEBUG
    list();
    #endif
    if(chunk_size == 0)
    {
        #ifdef DEBUG
        printf("\n\n-->Attempted to allocate 0 bytes\n");
        #endif   
        return NULL;
    }
    
    #ifdef DEBUG
    printf("\n\n-->Allocating %ld bytes\n", chunk_size);
    #endif   
    if(freed_list_head == NULL)
    {
        #ifdef DEBUG
        printf("-->Freed list empty...\n");
        #endif
        alloc_list_append(create_block(chunk_size));
        return alloc_list_tail->data;
    }

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
        return;
    }
    #ifdef DEBUG
    printf("\n\n-->Attempting to dealloc block with data %p...\n", chunk);
    #endif
    struct block* current_block = alloc_list_head;
    
    while(current_block != NULL)
    { 
        if(current_block->data == chunk)
        {  
            #ifdef DEBUG
            printf("-->Found the block (Block: %p, Next: %p, Prev: %p, Size: %ld, Data: %p)\n", 
            (void*) current_block, (void*) current_block->next, (void*) current_block->prev, 
            current_block->size, current_block->data);
            #endif
            list_delete(current_block);
            freed_list_append(current_block);
            return;
        }
        current_block = current_block->next;
    }

    #ifdef DEBUG
    printf("-->No valid block found...\n");
    #endif
    exit(1);
}

void set_stratergy(enum stratergy stratergy)
{
    current_stratergy = stratergy;
    
    #ifdef DEBUG
    printf("-->Stratergy changed: %d\n", current_stratergy);
    #endif
}

