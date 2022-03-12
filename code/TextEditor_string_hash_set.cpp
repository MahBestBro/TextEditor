#include "TextEditor_defs.h"
#include "TextEditor_string.h"
#include "TextEditor_string_hash_set.h"

#define HASHSET_BASE 31

StringHashSet InitHashSet()
{
    StringHashSet result;
    result.vals = HeapAllocZero(char*, INITIAL_HASHSET_SIZE);
    return result;
}

int Hash(int hashSetSize, char* val, int valLen)
{
    int value = 0;
    for (int i = 0; i < valLen; ++i)
        value = (value * HASHSET_BASE + val[i]) % hashSetSize;
    return value;
}

//Returns either index of val if in the hashset, else the index of a free slot in the hash set
int LinearProbe(StringHashSet* hashSet, char* val, int valLen)
{
    int hash = Hash(hashSet->size, val, valLen);
    for (int i = 0; i < hashSet->size; ++i)
    {
        int index = (hash + i) % hashSet->size;
		if (!hashSet->vals[index] || CompareStrings(hashSet->vals[index], 
                                                    StringLen(hashSet->vals[index]), 
                                                    val, 
                                                    valLen))
		{
			//if (i > 0)
			//	printf("Clash at index %i. Moved to index %i\n", hash, index);
			//else
			//	printf("String added at index %i\n", index);
			return index;
		}
    }

    Assert(false); //You shouldn't have gotten here
    return -1;
}

bool IsInHashSet(StringHashSet* hashSet, char* val, int valLen)
{
    int index = LinearProbe(hashSet, val, valLen);
    return hashSet->vals[index] != nullptr; 
}

//TODO: Realloc and rehashing
void AddToHashSet(StringHashSet* hashSet, char* val, int valLen)
{
    int index = LinearProbe(hashSet, val, valLen);
    if (!hashSet->vals[index])
    {
        //TODO: malloc at init instead so that we aren't doing heap allocations than we need to
        hashSet->vals[index] = (char*)malloc((valLen + 1) * sizeof(char));
        memcpy(hashSet->vals[index], val, valLen);
        hashSet->vals[index][valLen] = 0;

        hashSet->numVals++;
    }
}

void RemoveFromHashSet(StringHashSet* hashSet, char* val, int valLen)
{
    int index = LinearProbe(hashSet, val, valLen);
    if (hashSet->vals[index])
    {
        free(hashSet->vals[index]);
		hashSet->vals[index] = nullptr;
        hashSet->numVals--;

        index = (index + 1) % hashSet->size;
        while (hashSet->vals[index])
        {
            char* _val = hashSet->vals[index];
            hashSet->vals[index] = nullptr;
            int newIndex = LinearProbe(hashSet, _val, (int)strlen(_val));
            hashSet->vals[newIndex] = _val;
            index = (index + 1) % hashSet->size;
        }        
    }
}