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

struct Line
{
    char* text;
    int size;
    int len;
};

struct HighlightInfo
{
    int topHighlightStart = 0;
    int topHighlightLen = 0;
    int topLine = 0;
    int bottomHighlightEnd = 0;
    int bottomLine = 0;
    bool spansOneLine = false;
};

struct Editor
{
    char* fileName = nullptr;

    char currentChar = 0;
    Line lines[MAX_LINES];
    int numLines = 1;
    int topChangedLineIndex = -1;

    int cursorTextIndex = 0;
    int cursorLineIndex = 0;

    //Where the cursor was when user started highlighting
    int initialHighlightTextIndex = -1;
    int initialHighlightLineIndex = -1;

    IntPair textOffset = {};
};

void Init();
void Draw(float dt);
void Print(const char* message);

char* ReadEntireFile(char* fileName, uint32* fileLen  = nullptr);
bool WriteToFile(char* fileName, char* text, uint64 textLen, bool overwrite, int32 writeStart = 0);

void CopyToClipboard(const char* text, size_t len);
char* GetClipboardText();

char* ShowFileDialogAndGetFileName(bool save, size_t* fileNameLen = 0);

#endif
