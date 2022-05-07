#include "stdlib.h"

#include "TextEditor_defs.h"
#include "TextEditor_alloc.h"

StringArena temporaryStringArena;
StringArena undoStringArena;

internal LineMemoryArena lineMemory;

internal inline byte* StringArena_PrevBlock(void* block)
{
    return *((byte**)block - 1);
}

void* StringArena_Alloc(StringArena* arena, size_t size)
{
    Assert(arena->used + size + sizeof(byte*) <= MAX_STRING_ARENA_MEMORY);

    void* result = arena->top;
    arena->top += size;
    *(byte**)arena->top = (byte*)result;
    arena->top += sizeof(byte*);

    arena->used += size + sizeof(byte*);
    
    return result;
}

void* StringArena_Realloc(StringArena* arena, void* block, size_t size)
{
    Assert(block >= arena->memory && block <= arena->memory + MAX_STRING_ARENA_MEMORY);

    void* result = StringArena_Alloc(arena, size);
    memcpy(result, block, size);
    return result;
}

void StringArena_Free(StringArena* arena, void* block)
{
    Assert(block >= arena->memory && block <= arena->memory + MAX_STRING_ARENA_MEMORY);

    if (block > arena->top) return;

	if (block == arena->memory)
	{
		FlushStringArena(arena);
		return;
	}

    arena->used -= (size_t)(arena->top - StringArena_PrevBlock(block));
    arena->top = StringArena_PrevBlock(block);
}

void FlushStringArena(StringArena* arena)
{
    arena->top = arena->memory;
    arena->used = 0;
}



void* LineMemory_Alloc(size_t size)
{
    Assert(size % LINE_CHUNK_SIZE == 0);
    Assert(size > 0);

    const uint32 numChunksAllocating = (uint32)(size / LINE_CHUNK_SIZE);

    //If we have not allocated yet, allocate
    if (lineMemory.numUsedBlocks == 0)
    {
        Assert(lineMemory.numUsedChunks == 0);

        byte* result = lineMemory.memory;
        lineMemory.blockInfos[lineMemory.numUsedBlocks++] = {result, numChunksAllocating};
        lineMemory.numUsedChunks += numChunksAllocating;
        return result;
    }

    //Check if there's room in front
    if (MAX_LINE_MEM_CHUNKS - lineMemory.numUsedChunks >= numChunksAllocating)
    {
        LineMemoryBlockInfo topBlock = lineMemory.blockInfos[lineMemory.numUsedBlocks - 1];
        byte* result = topBlock.at + topBlock.numChunks * LINE_CHUNK_SIZE;
        lineMemory.blockInfos[lineMemory.numUsedBlocks++] = {result, numChunksAllocating};
        lineMemory.numUsedChunks += numChunksAllocating;
        return result;
    }
    
    //If no room in front, check if there's room in the back
    const uint32 numBytesAtFront = (uint32)(lineMemory.blockInfos[0].at - lineMemory.memory);
    if (numBytesAtFront / LINE_CHUNK_SIZE >= numChunksAllocating)
    {
        byte* result = lineMemory.memory;
        memcpy(lineMemory.blockInfos, lineMemory.blockInfos + 1, lineMemory.numUsedBlocks);
        lineMemory.blockInfos[0] = {result, numChunksAllocating};
        lineMemory.numUsedChunks += numChunksAllocating;
        return result;
    }

    //If no room in front or back, check if there's room in between blocks
    for (uint32 i = 0; i < lineMemory.numUsedBlocks - 1; ++i)
    {
        const LineMemoryBlockInfo currentBlock = lineMemory.blockInfos[i];
        const LineMemoryBlockInfo nextBlock = lineMemory.blockInfos[i + 1];

        const uint32 byteDiff = (uint32)(nextBlock.at - currentBlock.at);
        const uint32 freeChunks = byteDiff / LINE_CHUNK_SIZE + currentBlock.numChunks;
        if (freeChunks >= numChunksAllocating)
        {
            byte* result = currentBlock.at + currentBlock.numChunks * LINE_CHUNK_SIZE;
            //I don't think we need to do bounds checks here since we already did so?
            memcpy(lineMemory.blockInfos + i + 2, 
                   lineMemory.blockInfos + i + 1,
                   lineMemory.numUsedBlocks - i);
            lineMemory.blockInfos[i + 1] = {result, numChunksAllocating};
            lineMemory.numUsedBlocks++;
            lineMemory.numUsedChunks += numChunksAllocating;
            return result;
        }
    }

    //no room for such chunk
    return nullptr;
}

internal int LineMemoryBlockInfo_Compare(const void* a, const void* b)
{
    return (int)(((LineMemoryBlockInfo*)a)->at - ((LineMemoryBlockInfo*)b)->at);
}

internal LineMemoryBlockInfo* LineMemory_GetBlockInfoPtr(void* at)
{
    return (LineMemoryBlockInfo*)bsearch(&at, 
                                         lineMemory.blockInfos,
                                         lineMemory.numUsedBlocks,
                                         sizeof(LineMemoryBlockInfo),
                                         LineMemoryBlockInfo_Compare);
}

void* LineMemory_Realloc(void* block, size_t size)
{
    void* result = LineMemory_Alloc(size);
	if (result)
	{
        LineMemoryBlockInfo* blockInfo = LineMemory_GetBlockInfoPtr(block);
        Assert(blockInfo);

		memcpy(result, block, blockInfo->numChunks * LINE_CHUNK_SIZE);
		LineMemory_Free(block);
	}
    return result;
}

void LineMemory_Free(void* block)
{
    LineMemoryBlockInfo* blockPtr = LineMemory_GetBlockInfoPtr(block);
    const int blockIndex = (int)((blockPtr - lineMemory.blockInfos) / LINE_CHUNK_SIZE);

    lineMemory.numUsedChunks -= lineMemory.blockInfos[blockIndex].numChunks;
    memcpy(blockPtr, blockPtr + 1, lineMemory.numUsedBlocks - blockIndex - 1);
    lineMemory.numUsedBlocks--;
}