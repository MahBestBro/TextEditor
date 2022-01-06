#include "stdint.h"

#ifndef TEXT_EDITOR_DEFS_H
#define TEXT_EDITOR_DEFS_H

#define internal static
#define local_persist static 
#define global_variable static

#define Assert(cond) if (!(cond)) {*(int*)0 = 0;}

#define PIXEL_IN_BYTES 4

typedef uint8_t byte;
typedef uint8_t uchar;
typedef uint8_t uint8;
typedef uint32_t uint32;

typedef int32_t int32;
typedef int64_t int64;

typedef wchar_t wchar;

#endif