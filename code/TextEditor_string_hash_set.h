#ifndef TEXTEDITOR_STRING_HASH_SET_H
#define TEXTEDITOR_STRING_HASH_SET_H

#define INITIAL_HASHSET_SIZE 131

struct StringHashSet
{
    char** vals;
    int size = INITIAL_HASHSET_SIZE;
    int numVals = 0;
};

StringHashSet InitHashSet();
bool IsInHashSet(StringHashSet* hashSet, char* val, int valLen);
void AddToHashSet(StringHashSet* hashSet, char* val, int valLen);
void RemoveFromHashSet(StringHashSet* hashSet, char* val, int valLen);

#endif