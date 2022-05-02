#include "TextEditor_defs.h"
#include "TextEditor_alloc.h"

internal byte temp_string_memory[1 * MEGABYTE];
StringArena temporaryStringArena = {temp_string_memory, sizeof(temp_string_memory), 0};

internal LineMemoryArena lineMemory;


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

void InitLineMemory()
{
    for (int i = 0; i < MAX_LINE_MEMORY; i += LINE_CHUNK_SIZE)
        *(uint64*)(&lineMemory.memory[i]) = LINE_MEM_UNALLOCATED_CHUNK_SIGN;
}

//TODO: Maybe cache this in a header?
int LineMemory_ChunkSize(void* block)
{
    int result = 0;

    int startingChunk = (int)(((byte*)block - lineMemory.memory) / 128);
    Assert(startingChunk % LINE_CHUNK_SIZE == 0);
    for (int i = startingChunk; i < lineMemory.numTotalChunks; ++i)
    {
        int chunk = i * LINE_CHUNK_SIZE;
        if (*(uint64*)(&lineMemory.memory[chunk]) == LINE_MEM_UNALLOCATED_CHUNK_SIGN)
            break;
        result++;
    }

    return result;
}

void* LineMemory_Alloc(size_t size)
{
    Assert(size % LINE_CHUNK_SIZE == 0);

    const int numChunksAllocating = (int)(size / LINE_CHUNK_SIZE);
    for (int i = 0; i < lineMemory.numTotalChunks; ++i)
    {
        bool canAllocate = true;
        for (int j = 0; j < numChunksAllocating; ++j)
        {
            int chunk = (i + j) * LINE_CHUNK_SIZE;
            if (*(uint64*)(&lineMemory.memory[chunk]) != LINE_MEM_UNALLOCATED_CHUNK_SIGN)
            {
                canAllocate = false;
                break;
            }
        }

        if (canAllocate) 
        {
            //TODO: See whether we can actually store this as a smaller int (dk if it will be important tho)
            *(int*)(&lineMemory.memory[i * LINE_CHUNK_SIZE]) = numChunksAllocating;
            lineMemory.numChunksUsed += numChunksAllocating;
            return lineMemory.memory + i * LINE_CHUNK_SIZE;
        }
    }

    return nullptr;
}

void* LineMemory_Realloc(void* block, size_t size)
{
    void* result = LineMemory_Alloc(size);
	if (result)
	{
		memcpy(result, block, LineMemory_ChunkSize(block) * LINE_CHUNK_SIZE);
		LineMemory_Free(block);
	}
    return result;
}

void LineMemory_Free(void* block)
{
    const int blockChunkSize = LineMemory_ChunkSize(block);
    for (int i = 0; i < blockChunkSize * LINE_CHUNK_SIZE; i += LINE_CHUNK_SIZE)
        *(uint64*)(&lineMemory.memory[i]) = LINE_MEM_UNALLOCATED_CHUNK_SIGN;
    lineMemory.numChunksUsed -= blockChunkSize;
}