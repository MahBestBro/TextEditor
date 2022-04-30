#ifndef TEXT_EDITOR_ALLOC_H
#define TEXT_EDITOR_ALLOC_H

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

void* StringArena_Alloc(size_t size);
void* StringArena_Realloc(void* block, size_t size);
void StringArena_Free(void* block);
void FlushStringArena(StringArena* arena);

#endif