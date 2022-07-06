#include "stdio.h"
#include "stdlib.h"

#include "TextEditor_defs.h"
#include "TextEditor_input.h"
#include "TextEditor_string.h"

#ifndef TEXT_EDITOR_H
#define TEXT_EDITOR_H

struct ScreenBuffer
{
    void* memory;
    int width, height;
};

struct Rect
{
    int left, right, bottom, top;
};

struct Colour
{
    byte r, g, b;
};

inline bool operator==(Colour lhs, Colour rhs)
{
    return lhs.r == rhs.r && lhs.g == rhs.g && lhs.b == rhs.b; 
}

inline bool operator!=(Colour lhs, Colour rhs)
{
    return !(lhs == rhs); 
}

struct ColourRGBA
{
    byte r, g, b, a;
};

struct EditorPos
{
    int textAt;
    int line;
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
    string_buf text = {0}; //If backspacing, this will be in reverse
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
    string_buf fileName;

    char currentChar = 0;
    string_buf lines[MAX_LINES];
    int numLines = 1;
    int topChangedLineIndex = -1;

    EditorPos cursorPos = {0};

    //Where the cursor was when user started highlighting
    EditorPos highlightStart = {-1, -1};

    StringArena undoStringArena;
    UndoInfo undoStack[MAX_UNDOS];
    int numUndos = 0;
    UndoInfo redoStack[MAX_UNDOS];
    int numRedos = 0;

    IntPair textOffset = {};
};

typedef void (*EditorFunc)(Editor*);
typedef void (*VoidFunc)(void);

struct KeyCallback
{
    bool isEditorFunc;
    union
    {
        EditorFunc editorFunc;
        VoidFunc voidFunc;
    };
};

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

inline int RoundToInt(float x)
{
    float frac = x - (int)x;
    return (int)x + (frac >= 0.5f);
}

void FreeWin32(void* memory);

void* ReadEntireFile(string fileName, int* fileLen = nullptr);
inline void* ReadEntireFile(char* fileName, int* fileLen = nullptr)
{
    return ReadEntireFile(cstring(fileName), fileLen);
}
string ReadEntireFileAsString(string fileName);
bool WriteToFile(string fileName, string text, bool overwrite, int32 writeStart = 0);

void CopyToClipboard(string text);
string GetClipboardText();

string ShowFileDialogAndGetFileName(bool save);

void DrawText(string text, int xCoord, int yCoord, Colour colour, Rect limits = {0});

void OnTextChanged(); //TODO: Expand this to something like an array of function pointers
void OnFileOpen();
void OnEditorSwitch();

void HighlightSyntax(); //TODO: Rename to like Draw and then rename other Draw function to Update or something

#endif