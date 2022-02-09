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

struct IntPair
{
    int x, y;
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
};

//TODO: figure out a way to make this neater? 
struct UndoInfo
{
    EditorPos undoStart;
    EditorPos undoEnd;
    char** textByLine = nullptr;
    ResizableString reverseBuffer = {nullptr, INITIAL_LINE_SIZE, 0};
    int numLines = -1;
    UndoType type;
    bool wasHighlight;
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

    char currentChar = 0;
    Line lines[MAX_LINES];
    int numLines = 1;
    int topChangedLineIndex = -1;

    EditorPos cursorPos = {0};

    //Where the cursor was when user started highlighting
    EditorPos highlightStart = {-1, -1};

    bool lastActionWasUndo = false;
    UndoInfo undoStack[MAX_UNDOS];
    int numUndos = 0;
    UndoInfo redoStack[MAX_UNDOS];
    int numRedos = 0;

    IntPair textOffset = {};
};

#define AppendToDynamicArray(arr, len, val, size)   \
(arr)[(len)++] = (val);                             \
ResizeDynamicArray(&arr, len, sizeof(val), &size)   \

inline void ResizeDynamicArray(void* ptrToArr, int len, size_t elSize, int* size)
{
    if (len >= *size)
    {
        *size *= 2;
        *(void**)ptrToArr = realloc(*(void**)ptrToArr, *size * elSize);
    }
}



void Init();
void Draw(float dt);
void Print(const char* message);

char* ReadEntireFile(char* fileName, uint32* fileLen  = nullptr);
bool WriteToFile(char* fileName, char* text, uint64 textLen, bool overwrite, int32 writeStart = 0);

void CopyToClipboard(const char* text, size_t len);
char* GetClipboardText();

char* ShowFileDialogAndGetFileName(bool save, size_t* fileNameLen = 0);

#endif
