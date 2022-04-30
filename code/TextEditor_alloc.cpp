#include "TextEditor_defs.h"
#include "TextEditor_alloc.h"

internal byte temp_string_memory[4 * MEGABYTE];
StringArena temporaryStringArena = {temp_string_memory, sizeof(temp_string_memory), 0};


internal inline size_t StringArena_BlockSize(void* block)
{
    return *((size_t*)block - 1);
}

void* StringArena_Alloc(size_t size)
{
    Assert(temporaryStringArena.used + size + sizeof(size_t) <= temporaryStringArena.size);

    void* result = temporaryStringArena.top;
    *(size_t*)result = size; 
    result = (size_t*)result + 1;
    temporaryStringArena.top += size + sizeof(size_t);
    temporaryStringArena.used += size + sizeof(size_t);
    return result;
}

void* StringArena_Realloc(void* block, size_t size)
{
    void* result = StringArena_Alloc(size);
    memcpy(result, block, StringArena_BlockSize(block));
    return result;
}

void StringArena_Free(void* block)
{
    UNIMPLEMENTED("TODO: Avoid having to code this cause like this is useless from what I can tell");
    return;
}

void FlushStringArena()
{
    temporaryStringArena.top = temp_string_memory;
    temporaryStringArena.used = 0;
}