#include "TextEditor_defs.h"

#ifndef TEXT_EDITOR_ALLOC_H
#define TEXT_EDITOR_ALLOC_H

#define MAX_STRING_ARENA_MEMORY 100 * KILOBYTE
#define MAX_LINE_MEMORY 1 * MEGABYTE
#define MAX_LINE_MEM_CHUNKS MAX_LINE_MEMORY / LINE_CHUNK_SIZE

#define DEF_STRING_ARENA_FUNCS(arena)           \
inline void* Alloc_##arena(size_t size) \
{   \
    return StringArena_Alloc(&(arena), size);  \
}   \
inline void* Realloc_##arena(void* block, size_t size)  \
{   \
    return StringArena_Realloc(&(arena), block, size); \
}   \
inline void Free_##arena(void* block)    \
{   \
    return StringArena_Free(&(arena), block); \
}   \
const Allocator allocator_##arena = {Alloc_##arena, Realloc_##arena, Free_##arena};

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
    byte memory[MAX_STRING_ARENA_MEMORY];
    byte* top = memory;
    size_t used = 0; //For debugging only
};

struct LineMemoryBlockInfo
{
    byte* at;
    uint32 numChunks; //TODO: Maybe this can be uint16?
};

struct LineMemoryArena
{
    byte memory[MAX_LINE_MEMORY];
    LineMemoryBlockInfo blockInfos[MAX_LINE_MEM_CHUNKS];
    uint32 numUsedBlocks = 0;
    uint32 numUsedChunks = 0;
};

void* StringArena_Alloc(StringArena* arena, size_t size);
void* StringArena_Realloc(StringArena* arena, void* block, size_t size);
void StringArena_Free(StringArena* arena, void* block);
void FlushStringArena(StringArena* arena);


void InitLineMemory();
void* LineMemory_Alloc(size_t size);
void* LineMemory_Realloc(void* block, size_t size);
void LineMemory_Free(void* block);

#endif