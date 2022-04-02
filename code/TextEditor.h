#include "stdio.h"
#include "stdlib.h"

#include "TextEditor_defs.h"
#include "TextEditor_input.h"

#ifndef TEXT_EDITOR_H
#define TEXT_EDITOR_H

struct ScreenBuffer
{
    void* memory;
    int width, height;
};

struct FontChar
{
    uint32 width, height; // Size of glyph
    uint32 left, top;     // Offset from baseline to left/top of glyph
    uint32 advance;       // Offset to advance to next glyph
    void* pixels;
};

struct Colour
{
    byte r, g, b;
};

struct ColourRGBA
{
    byte r, g, b, a;
};

struct EditorPos
{
    int textAt;
    int line;
};

struct Line
{
    char* text;
    int size;
    int len;
};

struct ResizableString
{
    char* buffer;
    int size;
    int len;
};

struct TextSectionInfo
{
    EditorPos top;
    EditorPos bottom;
    int topLen = 0;
    bool spansOneLine = false;
};

enum UndoType
{
    UNDOTYPE_ADDED_TEXT,
    UNDOTYPE_OVERWRITE,
    UNDOTYPE_REMOVED_TEXT_REVERSE_BUFFER,
    UNDOTYPE_REMOVED_TEXT_SECTION,
    UNDOTYPE_MULTILINE_ADD,
    UNDOTYPE_MULTILINE_REMOVE
};

//TODO: figure out a way to make this neater? 
struct UndoInfo
{
    UndoType type;
    EditorPos start;
    EditorPos end;
    EditorPos prevCursorPos;

    bool wasHighlight = false; //Only used for UNDOTYPE_REMOVED_TEXT_SECTION
    
    char** textByLine = nullptr;
    int numLines = -1;
    
    //Only used for backspacing char by char
    ResizableString reverseBuffer = {nullptr, INITIAL_LINE_SIZE, 0};
};

inline bool operator ==(EditorPos lhs, EditorPos rhs)
{
    return lhs.textAt == rhs.textAt && lhs.line == rhs.line;
}

inline bool operator !=(EditorPos lhs, EditorPos rhs)
{
    return !(lhs == rhs);
}

struct Editor
{
    char* fileName = nullptr;
    int fileNameLen = 0;

    char currentChar = 0;
    Line lines[MAX_LINES];
    int numLines = 1;
    int topChangedLineIndex = -1;

    EditorPos cursorPos = {0};

    //Where the cursor was when user started highlighting
    EditorPos highlightStart = {-1, -1};

    UndoInfo undoStack[MAX_UNDOS];
    int numUndos = 0;
    UndoInfo redoStack[MAX_UNDOS];
    int numRedos = 0;

    IntPair textOffset = {};
};

#define AppendToDynamicArray(arr, len, val, size)   \
(arr)[(len)++] = (val);                             \
ResizeDynamicArray(&arr, len, sizeof(val), &size)   \

//NOTE: Always call this before you add to the array or else you will lose any elements whose index > size
inline void ResizeDynamicArray(void* ptrToArr, int len, size_t elSize, int* size)
{
    bool shouldRealloc = false;
    while (len >= *size)
    {
        shouldRealloc = true;
        *size *= 2;
    }
    if (shouldRealloc) *(void**)ptrToArr = realloc(*(void**)ptrToArr, *size * elSize); 
}

void Init();
void Draw(float dt);
void Print(const char* message);

inline void* dbg_malloc(size_t size, const char* file, int line)
{
    char msg[128];
    snprintf(msg, sizeof(msg), "Allocated %zi bytes at %s, Line: %i.\n", size, file, line);
    Print(msg);
    return malloc(size);
}

void FreeWin32(void* memory);

char* ReadEntireFile(char* fileName, uint32* fileLen  = nullptr);
bool WriteToFile(char* fileName, char* text, uint64 textLen, bool overwrite, int32 writeStart = 0);

void CopyToClipboard(const char* text, size_t len);
char* GetClipboardText();

char* ShowFileDialogAndGetFileName(bool save, int* fileNameLen = 0);

#endif