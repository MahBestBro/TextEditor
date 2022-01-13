#ifndef UNICODE
#define UNICODE
#endif

#include <windows.h> 
#include "stdio.h"

#include <ft2build.h>
#include FT_FREETYPE_H 

#include "TextEditor_defs.h"
#include "TextEditor.h"
#include "TextEditor_input.h"

Input input = {};
FontChar fontChars[128];
ScreenBuffer screenBuffer;

global_variable BITMAPINFO bitmapInfo;
global_variable bool running;

wchar* CStrToWStr(const char *c)
{
    const size_t cSize = strlen(c)+1;
    wchar* wc = (wchar*)malloc(cSize * sizeof(wchar));
    mbstowcs_s(0, wc, cSize, c, cSize+1);

    return wc;
}

inline void win32_HandleInputDown(byte* inputFlags)
{
    if (!InputHeld(*inputFlags)) 
    {
        *inputFlags |= INPUT_DOWN;
        *inputFlags |= INPUT_HELD;
    }
}

inline void win32_HandleInputUp(byte* inputFlags)
{
    *inputFlags &= ~INPUT_HELD;
    *inputFlags |= INPUT_UP;
}

internal void win32_ProcessInput()
{
    for (int i = 0; i < NUM_INPUTS; ++i)
    {
        if (InputDown(input.flags[i]))
        {
            input.flags[i] &= ~INPUT_DOWN;
        }
        else if (InputUp(input.flags[i]))
        {
            input.flags[i] &= ~INPUT_UP;
        }
    }
}

void win32_LogInput(InputCode code)
{
	if (InputDown(input.flags[code]))
	{
		Assert(true);
	}

    wchar inputLog[32];
    wchar* codeStr = CStrToWStr(InputCodeToStr(code));
    if (code >= MOUSE_START && code < ARROWS_START)
        swprintf_s(inputLog, L"%s BUTTON", codeStr);
    else if (code >= ARROWS_START && code < NON_SHIFT_START)
        swprintf_s(inputLog, L"%s ARROW KEY", codeStr);
    else
        swprintf_s(inputLog, L"%s KEY", codeStr);
    free(codeStr);

    wchar log[64];
    if (InputDown(input.flags[code]))
        swprintf_s(log, L"%s DOWN\n", inputLog);
    else if (InputUp(input.flags[code]))
        swprintf_s(log, L"%s UP\n", inputLog);
    else if (InputHeld(input.flags[code]))
        swprintf_s(log, L"HOLDING %s\n", inputLog);

    if (input.flags[code])
        OutputDebugString(log);
}

//DIB: Device Independent Bitmap
internal void win32_ResizeDIB(int width, int height)
{
    if (screenBuffer.memory)
    {
        VirtualFree(screenBuffer.memory, 0, MEM_RELEASE);
    }

    screenBuffer.width = width;
    screenBuffer.height = height;

    bitmapInfo.bmiHeader.biSize        = sizeof(bitmapInfo.bmiHeader);
    bitmapInfo.bmiHeader.biWidth       = screenBuffer.width;
    bitmapInfo.bmiHeader.biHeight      = screenBuffer.height;
    bitmapInfo.bmiHeader.biPlanes      = 1;
    bitmapInfo.bmiHeader.biBitCount    = 32;
    bitmapInfo.bmiHeader.biCompression = BI_RGB;

    int bitmapMemorySize = PIXEL_IN_BYTES * screenBuffer.width * screenBuffer.height;
    screenBuffer.memory = VirtualAlloc(0, bitmapMemorySize, MEM_COMMIT, PAGE_READWRITE);   

}

internal void win32_UpdateWindow(HDC deviceContext, RECT* windowRect, 
                                 int x, int y, int width, int height)
{
    int windowWidth = windowRect->right - windowRect->left;
    int windowHeight = windowRect->bottom - windowRect->top;
    StretchDIBits(
        deviceContext, 
        0, 0, screenBuffer.width, screenBuffer.height,
        0, 0, windowWidth, windowHeight,
        screenBuffer.memory,
        &bitmapInfo,
        DIB_RGB_COLORS,
        SRCCOPY);
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, 
                    PWSTR pCmdLine, int nCmdShow)
{
    LARGE_INTEGER perfCountFreqResult;
    QueryPerformanceFrequency(&perfCountFreqResult);
    int64 perfCountFreq = perfCountFreqResult.QuadPart;


    // Register the window class.
    const wchar windowName[]  = L"TextEditorWindowClass";
    
    WNDCLASS wc = {};
    wc.lpfnWndProc   = WindowProc;
    wc.hInstance     = hInstance;
    wc.lpszClassName = windowName;

    RegisterClass(&wc);

    // Create the window.

    HWND hwnd = CreateWindowEx(
        0,                              // Optional window styles.
        windowName,                     // Window class
        L"Text editor",    // Window text
        WS_OVERLAPPEDWINDOW | WS_VISIBLE,            // Window style

        // Size and position
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,

        NULL,       // Parent window    
        NULL,       // Menu
        hInstance,  // Instance handle
        NULL        // Additional application data
        );

    if (hwnd == NULL)
    {
        return 0;
    }

    //Init font stuff
    FT_Library ft;
    if (FT_Init_FreeType(&ft))
    {
        OutputDebugString(L"ERROR::FREETYPE: Could not init FreeType Library\n");
        return -1;
    }
    FT_Face face;
    if (FT_New_Face(ft, "fonts/consolab.ttf", 0, &face))
    {
        OutputDebugString(L"ERROR::FREETYPE: Failed to load font\n");  
        return -1;
    }
    FT_Set_Pixel_Sizes(face, 0, FONT_SIZE);  

    for (uchar c = 0; c < 128; ++c)
    {
        FontChar fc;
        if (FT_Load_Char(face, c, FT_LOAD_RENDER))
        {
            OutputDebugString(L"ERROR::FREETYTPE: Failed to load Glyph\n");
            continue;
        }

        fc.width = face->glyph->bitmap.width;
        fc.height = face->glyph->bitmap.rows;
        fc.left = face->glyph->bitmap_left;
        fc.top = face->glyph->bitmap_top;
        fc.advance = face->glyph->advance.x;
        fc.pixels = malloc(fc.width * fc.height); //Each byte refers to 1 pixel (8bpp)
        memcpy(fc.pixels, face->glyph->bitmap.buffer, fc.width * fc.height);
        fontChars[c] = fc;
    }

    ShowWindow(hwnd, nCmdShow);

    running = true;
    int64 prevCount = 0;
    while (running)
    { 
        LARGE_INTEGER currentCountResult;
        QueryPerformanceCounter(&currentCountResult);
        int64 currentCount = currentCountResult.QuadPart;
        float deltaTime = (float)(currentCount - prevCount) / (float)perfCountFreq;
        //int64 fps = perfCountFreq / (currentCount - prevCount);
        prevCount = currentCount;

        //wchar log[100];
        //swprintf_s(log, L"dt: %fs, FPS: %lli\n", deltaTime, fps);
        //OutputDebugString(log);

        //for (int i = 0; i < NUM_INPUTS; ++i)
        //    win32_LogInput((InputCode)i);

        win32_ProcessInput();

        MSG msg;
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT)
                running = false;

            TranslateMessage(&msg);
            DispatchMessageA(&msg);
        }

        wchar inputLog[64];

        Draw(deltaTime);
        
        HDC hdc = GetDC(hwnd);
        RECT windowRect;
        GetClientRect(hwnd, &windowRect);
        int windowWidth = windowRect.right - windowRect.left;
        int windowHeight = windowRect.bottom - windowRect.top;
        win32_UpdateWindow(hdc, &windowRect, 0, 0, windowWidth, windowHeight);
        ReleaseDC(hwnd, hdc);
    }
    return 0;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_SIZE:
        {
            RECT clientRect;
            GetClientRect(hwnd, &clientRect);
            int width = clientRect.right - clientRect.left;
            int height = clientRect.bottom - clientRect.top;
            win32_ResizeDIB(width, height);
        } return 0;

        case WM_CLOSE:
        {
            running = false;
            DestroyWindow(hwnd);
        } return 0;

        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;

        case WM_PAINT:
        {
            PAINTSTRUCT paint;
            HDC hdc = BeginPaint(hwnd, &paint);

            
            int x = paint.rcPaint.left;
            int y = paint.rcPaint.top;
            int width = paint.rcPaint.right - paint.rcPaint.left;
            int height = paint.rcPaint.bottom - paint.rcPaint.top;
            //FillRect(hdc, &ps.rcPaint, (HBRUSH)(COLOR_WINDOW+1));
            RECT clientRect;
            GetClientRect(hwnd, &clientRect);
            win32_UpdateWindow(hdc, &clientRect, x, y, width, height);


            EndPaint(hwnd, &paint);
        } return 0;

        case WM_LBUTTONDOWN:
            win32_HandleInputDown(&input.leftMouse);
            return 0;

        case WM_LBUTTONUP:
            win32_HandleInputUp(&input.leftMouse);
            return 0;

        case WM_RBUTTONDOWN:
            win32_HandleInputDown(&input.rightMouse);
            return 0;

        case WM_RBUTTONUP:
            win32_HandleInputUp(&input.rightMouse);
            return 0;

        case WM_MBUTTONDOWN:
            win32_HandleInputDown(&input.middleMouse);
            return 0;

        case WM_MBUTTONUP:
            win32_HandleInputUp(&input.middleMouse);
            return 0;

        case WM_SYSKEYDOWN:
        case WM_KEYDOWN:
        {
            uint32 vkCode = (uint32)wParam;
            if (vkCode >= (uint32)'A' && vkCode <= (uint32)'Z')
                win32_HandleInputDown(&input.letterKeys[vkCode - 'A']);
            else if (vkCode >= (uint32)'0' && vkCode <= (uint32)'9')
                win32_HandleInputDown(&input.numberKeys[vkCode - '0']);
            else if (vkCode >= VK_LEFT && vkCode <= VK_DOWN)
                win32_HandleInputDown(&input.arrowKeys[vkCode - VK_LEFT]);
            else if (vkCode == VK_BACK)
                win32_HandleInputDown(&input.backspace);
            else if (vkCode == VK_SHIFT || vkCode == VK_LSHIFT)
                win32_HandleInputDown(&input.leftShift);
            else if (vkCode == VK_MENU || vkCode == VK_LMENU)
                win32_HandleInputDown(&input.leftAlt);
            else if (vkCode == VK_CAPITAL)
                win32_HandleInputDown(&input.capsLock);
            else if (vkCode == VK_TAB)
                win32_HandleInputDown(&input.tab);
            else if (vkCode == VK_RETURN)
                win32_HandleInputDown(&input.enter);
            else if (vkCode == VK_CONTROL || vkCode == VK_LCONTROL)
                win32_HandleInputDown(&input.leftCtrl);
            else if (vkCode == VK_SPACE)
                win32_HandleInputDown(&input.space);
            else if (vkCode == VK_OEM_4)
                win32_HandleInputDown(&input.flags[INPUTCODE_OPEN_SQ_BRACKET]);
            else if (vkCode == VK_OEM_6)
                win32_HandleInputDown(&input.flags[INPUTCODE_CLOSED_SQ_BRACKET]);
            else if (vkCode == VK_OEM_1)
                win32_HandleInputDown(&input.flags[INPUTCODE_SEMICOLON]);
            else if (vkCode == VK_OEM_COMMA) 
                win32_HandleInputDown(&input.flags[INPUTCODE_COMMA]);
            else if (vkCode == VK_OEM_PERIOD) 
                win32_HandleInputDown(&input.flags[INPUTCODE_PERIOD]);
            else if (vkCode == VK_OEM_7) 
                win32_HandleInputDown(&input.flags[INPUTCODE_QUOTE]);
            else if (vkCode == VK_OEM_2) 
                win32_HandleInputDown(&input.flags[INPUTCODE_FORWARD_SLASH]);
            else if (vkCode == VK_OEM_3) 
                win32_HandleInputDown(&input.grave);
            else if (vkCode == VK_OEM_PLUS) 
                win32_HandleInputDown(&input.equals);
            else if (vkCode == VK_OEM_MINUS) 
                win32_HandleInputDown(&input.minus);
            else if (vkCode == VK_OEM_5) 
                win32_HandleInputDown(&input.backslash);
        } return 0;

        case WM_SYSKEYUP:
        case WM_KEYUP:
        {
            uint32 vkCode = (uint32)wParam;
            if (vkCode >= (uint32)'A' && vkCode <= (uint32)'Z')
                win32_HandleInputUp(&input.letterKeys[vkCode - 'A']);
            else if (vkCode >= (uint32)'0' && vkCode <= (uint32)'9')
                win32_HandleInputUp(&input.numberKeys[vkCode - '0']);
            else if (vkCode >= VK_LEFT && vkCode <= VK_DOWN)
                win32_HandleInputUp(&input.arrowKeys[vkCode - VK_LEFT]);
            else if (vkCode == VK_BACK)
                win32_HandleInputUp(&input.backspace);
            else if (vkCode == VK_SHIFT || vkCode == VK_LSHIFT)
                win32_HandleInputUp(&input.leftShift);
            else if (vkCode == VK_MENU || vkCode == VK_LMENU)
                win32_HandleInputUp(&input.leftAlt);
            else if (vkCode == VK_CAPITAL)
                win32_HandleInputUp(&input.capsLock);
            else if (vkCode == VK_TAB)
                win32_HandleInputUp(&input.tab);
            else if (vkCode == VK_RETURN)
                win32_HandleInputUp(&input.enter);
            else if (vkCode == VK_CONTROL || vkCode == VK_LCONTROL)
                win32_HandleInputUp(&input.leftCtrl);
            else if (vkCode == VK_SPACE)
                win32_HandleInputUp(&input.space);
            else if (vkCode == VK_OEM_4)
                win32_HandleInputUp(&input.flags[INPUTCODE_OPEN_SQ_BRACKET]);
            else if (vkCode == VK_OEM_6)
                win32_HandleInputUp(&input.flags[INPUTCODE_CLOSED_SQ_BRACKET]);
            else if (vkCode == VK_OEM_1)
                win32_HandleInputUp(&input.flags[INPUTCODE_SEMICOLON]);
            else if (vkCode == VK_OEM_COMMA) 
                win32_HandleInputUp(&input.flags[INPUTCODE_COMMA]);
            else if (vkCode == VK_OEM_PERIOD) 
                win32_HandleInputUp(&input.flags[INPUTCODE_PERIOD]);
            else if (vkCode == VK_OEM_7) 
                win32_HandleInputUp(&input.flags[INPUTCODE_QUOTE]);
            else if (vkCode == VK_OEM_2) 
                win32_HandleInputUp(&input.flags[INPUTCODE_FORWARD_SLASH]);
            else if (vkCode == VK_OEM_3) 
                win32_HandleInputUp(&input.grave);
            else if (vkCode == VK_OEM_PLUS) 
                win32_HandleInputUp(&input.equals);
            else if (vkCode == VK_OEM_MINUS) 
                win32_HandleInputUp(&input.minus);
            else if (vkCode == VK_OEM_5) 
                win32_HandleInputUp(&input.backslash);
        } return 0;
        
        

    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}
