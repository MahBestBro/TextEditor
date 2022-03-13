#ifndef TEXTEDITOR_STRING_HASH_SET_H
#define TEXTEDITOR_STRING_HASH_SET_H





StringHashSet InitHashSet();
bool IsInHashSet(StringHashSet* hashSet, char* val, int valLen);
void AddToHashSet(StringHashSet* hashSet, char* val, int valLen);
void RemoveFromHashSet(StringHashSet* hashSet, char* val, int valLen);

#endif