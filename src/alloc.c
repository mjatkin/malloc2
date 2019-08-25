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
    #ifdef DEBUG
    printf("-->Creating new block...\n");
    #endif

    struct block* current_block = 
        (struct block*) change_break(sizeof(struct block));
    
    current_block->next = NULL;
    current_block->prev = NULL;
    current_block->size = chunk_size;
    current_block->data = change_break(chunk_size);

    #ifdef DEBUG
    printf("-->Created block (Next: %p, Prev: %p, Size: %ld, Data: %p)\n", 
            (void*) current_block->next, (void*) current_block->prev,
            current_block->size, current_block->data);
    #endif
    return current_block;
}

static void alloc_list_append(struct block* block)
{
    #ifdef DEBUG
    printf("-->Appending block (Next: %p, Prev: %p, Size: %ld, Data: %p) to"
            " back of alloc_list.\n", 
            (void*) block->next, (void*) block->prev, block->size, block->data);
    #endif

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
}

static void freed_list_append(struct block* block)
{
    #ifdef DEBUG
    printf("-->Appending block (Next: %p, Prev: %p, Size: %ld, Data: %p) to"
            " back of freed_list.\n", 
            (void*) block->next, (void*) block->prev, block->size, block->data);
    #endif
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
}

static void list_delete(struct block* block)
{
    #ifdef DEBUG
    printf("-->Removing block (Next: %p, Prev: %p, Size: %ld, Data: %p) to\n", 
            (void*) block->next, (void*) block->prev, block->size, block->data);
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
        block->next = NULL;
    }
    if(block->prev != NULL)
    {
        block->prev->next = block->next;
        block->prev = NULL;
    }
}

static void* alloc_first(size_t chunk_size)
{
    struct block* current_block = freed_list_head;
    
    while(current_block->next != NULL)
    {
        if(current_block->size >= chunk_size)
        {   
            list_delete(current_block);
            alloc_list_append(current_block);
            return current_block->data;
        }
        current_block = current_block->next;
    }

    #ifdef DEBUG
    printf("-->No valid block found...");
    #endif
    return (create_block(chunk_size)->data);
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
    struct block* current_block = alloc_list_head;
    
    do
    {
            printf("Current1: %p\n", current_block->data); 
            printf("Current2: %p\n", chunk); 
        if(current_block->data == chunk)
        {  
            list_delete(current_block);
            freed_list_append(current_block);
            return;
        }
        current_block = current_block->next;
    } while(current_block->next != NULL);

    #ifdef DEBUG
    printf("-->No valid block found...");
    #endif
    exit(3);
}

void set_stratergy(enum stratergy stratergy)
{
    current_stratergy = stratergy;
    
    #ifdef DEBUG
    printf("-->Stratergy changed: %d\n", current_stratergy);
    #endif
}

