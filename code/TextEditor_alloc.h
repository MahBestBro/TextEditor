#include "TextEditor_defs.h"

#ifndef TEXT_EDITOR_ALLOC_H
#define TEXT_EDITOR_ALLOC_H

#define MAX_LINE_MEMORY 2 * MEGABYTE
#define LINE_MEM_UNALLOCATED_CHUNK_SIGN 0x0404040404040404

typedef void* (*alloc_func)(size_t);
typedef void* (*realloc_func)(void*, size_t);
typedef void (*free_func)(void*);

struct Allocator
{
    alloc_func Alloc = malloc;
    realloc_func Realloc = realloc;
    free_func Free = free;
};

struct StringArena
{
    byte* top;
    size_t size;
    size_t used;
};

struct LineMemoryArena
{
    byte memory[MAX_LINE_MEMORY];
    size_t numTotalChunks = MAX_LINE_MEMORY / LINE_CHUNK_SIZE;
    size_t numChunksUsed = 0;

};

void* StringArena_Alloc(size_t size);
void* StringArena_Realloc(void* block, size_t size);
void StringArena_Free(void* block);
void FlushStringArena(StringArena* arena);

void InitLineMemory();
void* LineMemory_Alloc(size_t size);
void* LineMemory_Realloc(void* block, size_t size);
void LineMemory_Free(void* block);

#endif