#ifndef TEXT_EDITOR_DYNARRAY_H
#define TEXT_EDITOR_DYNARRAY_H


#define AppendToDynamicArray(arr, len, val, size)   \
(arr)[(len)++] = (val);                             \
ResizeDynamicArray(&arr, len, sizeof(val), &size)   \

//NOTE: Always call this before you add to the array or else you will lose any elements whose index > size
inline void ResizeDynamicArray(void* ptrToArr, size_t len, size_t elSize, int* size)
{
    bool shouldRealloc = false;
    while (len >= *size)
    {
        shouldRealloc = true;
        *size *= 2;
    }
    if (shouldRealloc) *(void**)ptrToArr = realloc(*(void**)ptrToArr, *size * elSize); 
}

inline void ResizeDynamicArray(void* ptrToArr, size_t len, size_t elSize, size_t* size)
{
    bool shouldRealloc = false;
    while (len >= *size)
    {
        shouldRealloc = true;
        *size *= 2;
    }
    if (shouldRealloc) *(void**)ptrToArr = realloc(*(void**)ptrToArr, *size * elSize); 
}


#endif