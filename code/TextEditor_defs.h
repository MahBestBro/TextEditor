#include "stdint.h"

#ifndef TEXT_EDITOR_DEFS_H
#define TEXT_EDITOR_DEFS_H

#define internal static
#define local_persist static 
#define global_variable static

#define __Out

#define Assert(cond) if (!(cond)) {*(int*)0 = 0;}
#define UNIMPLEMENTED(message) Assert(false)

#define Xor(a, b) ( ((a) && !(b)) || (!(a) && (b)) )

#define StackArrayLen(arr) (sizeof((arr)) / sizeof((arr)[0]))

#define Clamp(x, lo, hi) (max(min((x), (hi)), (lo)))
#define InRange(x, lo, hi) ((x) >= (lo) && (x) <= (hi))

//#define HeapAlloc(type, numEls) (type*)dbg_malloc((numEls) * sizeof(type), __FILE__, __LINE__)
#define HeapAlloc(type, numEls) (type*)malloc((numEls) * sizeof(type))
#define HeapAllocZero(type, numEls) (type*)calloc((numEls), sizeof(type))
#define HeapRealloc(type, arr, numEls) (type*)realloc(arr, (numEls) * sizeof(type))

#define INTROSPECT(introspectFunctionName)

#define KILOBYTE 1000
#define MEGABYTE (1000 * KILOBYTE)
#define GIGABYTE (1000 * MEGABYTE)

#define PIXEL_IN_BYTES 4

#define MAX_LINES 256
#define MAX_UNDOS 256
#define LINE_CHUNK_SIZE 128

#define TEXT_START IntPair  \
{   \
    36 + MAX_LINE_NUM_DIGITS * (int)fontData.chars['0'].advance,    \
    screenBuffer.height - (int)fontData.maxHeight - 5   \
}   \

#define TEXT_LIMITS Rect    \
{   \
    TEXT_START.x,   \
    screenBuffer.width - 10,    \
    (int)(fontData.maxHeight + fontData.lineGap),   \
    screenBuffer.height - 1 \
}   \

typedef uint8_t byte;
typedef uint8_t uchar;

typedef uint8_t uint8;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef int32_t int32;
typedef int64_t int64;

typedef wchar_t wchar;

#endif