#include "stdint.h"

#ifndef TEXT_EDITOR_WIN32_H
#define TEXT_EDITOR_WIN32_H

#define internal static
#define local_persist static 
#define global_variable static

typedef uint8_t byte;
typedef uint32_t uint32;

typedef wchar_t wchar;

#endif
