/*
 * Implementation of the memory allocator.
 *
 * Matthew Atkin
 * s3603797
 * 
 * Aug 2019
 */
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

static void* change_break(size_t chunk_size)
{
    #ifdef DEBUG
    printf("-->Moving program break %ld bytes forward. (%p -> %p)\n", chunk_size, sbrk(0), ((char*) sbrk(0)) + chunk_size);
    #endif
    return sbrk(chunk_size);
}

static struct block* create_block()
{
    #ifdef DEBUG
    printf("-->Creating new block...\n");
    #endif
    return (struct block*) change_break(sizeof(struct block));
}

static void freed_list_init()
{
    #ifdef DEBUG
    printf("-->Initialising freed list...\n");
    #endif
    freed_list_head = create_block();
    freed_list_tail = freed_list_head;
}

static void alloc_list_init()
{
    #ifdef DEBUG
    printf("-->Initialising alloc list...\n");
    #endif
    alloc_list_head = create_block();
    alloc_list_tail = alloc_list_head;
}

static void alloc_list_add_back()
{

}

static void freed_list_add_back()
{

}

static void* alloc_first(size_t chunk_size)
{
    struct block* current_block = freed_list_head;
    
    while(current_block->next != NULL)
    {
        if(current_block->size >= chunk_size)
        {   
            /* HERE! */
            alloc_list_add_back(current_block);
            list_delete(current_block);
            return current_block->data;
        }
        current_block = current_block->next;
    }

    #ifdef DEBUG
    printf("-->No valid block found...");
    #endif

    alloc_list_add_back();
}

static void* alloc_best(size_t chunk_size)
{
    return NULL;
}

static void* alloc_worst(size_t chunk_size)
{
    return NULL;
}

void* alloc(size_t chunk_size)
{
    if(chunk_size == 0)
    {
        #ifdef DEBUG
        printf("-->Attempted to allocate 0 bytes\n");
        #endif   
        return NULL;
    }
    
    if(freed_list_head == NULL)
    {
        #ifdef DEBUG
        printf("-->Freed list empty...\n");
        #endif
        alloc_list_init();
        alloc_list_head->data = change_break(chunk_size);
        alloc_list_head->size = chunk_size;
        alloc_list_head->next = NULL;
        alloc_list_head->prev = NULL;
        return alloc_list_head->data;
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
    
}

void set_stratergy(enum stratergy stratergy)
{
    current_stratergy = stratergy;
    
    #ifdef DEBUG
    printf("-->Stratergy changed: %d\n", current_stratergy);
    #endif
}

