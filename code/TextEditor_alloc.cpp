#include "stdlib.h"

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

//void InitLineMemory(uint32 initialNumberOfChunks) //TODO: Maybe this can be uint16?
//{
//    for (int i = 0; i < MAX_LINE_MEMORY; i += LINE_CHUNK_SIZE)
//        *(uint64*)(&lineMemory.memory[i]) = LINE_MEM_UNALLOCATED_CHUNK_SIGN;
//}

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