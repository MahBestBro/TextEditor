#ifndef UNICODE
#define UNICODE
#endif

#include <windows.h> 
#include "stdio.h"
#include "TextEditor_win32.h"

#define PIXEL_IN_BYTES 4

#define INPUT_DOWN 0b001
#define INPUT_UP   0b010
#define INPUT_HELD 0b100

#define NUM_INPUTS 39

union Input
{
    struct
    {
        byte leftMouse;
        byte rightMouse;
        byte middleMouse;
        byte letterKeys[26];
        byte numberKeys[10];
    };
    byte flags[NUM_INPUTS];
};

global_variable Input input = {};

global_variable BITMAPINFO bitmapInfo;
global_variable void* bitmapMemory;
global_variable int bitmapWidth;
global_variable int bitmapHeight;
global_variable bool running;

inline bool InputDown(byte inputFlags)
{
    return (inputFlags & INPUT_DOWN) == INPUT_DOWN;
}

inline bool InputUp(byte inputFlags)
{
    return (inputFlags & INPUT_UP) == INPUT_UP;
}

inline bool InputHeld(byte inputFlags)
{
    return (inputFlags & INPUT_HELD) == INPUT_HELD;
}

internal void ProcessInput()
{
    for (int i = 0; i < NUM_INPUTS; ++i)
    {
        if (InputDown(input.flags[i]))
        {
            input.flags[i] &= ~INPUT_DOWN;
            input.flags[i] |= INPUT_HELD;
        }
        else if (InputUp(input.flags[i]))
        {
            input.flags[i] &= ~INPUT_UP;
        }
    }
}

inline void win32_HandleInputDown(byte* inputFlags)
{
    *inputFlags |= INPUT_DOWN;
    *inputFlags |= INPUT_HELD;
}

inline void win32_HandleInputUp(byte* inputFlags)
{
    *inputFlags &= ~INPUT_HELD;
    *inputFlags |= INPUT_UP;
}

internal void Draw(int xOffset, int yOffset)
{
    int width = bitmapWidth;
    int height = bitmapHeight;

    int pitch = width * PIXEL_IN_BYTES;
    byte* row = (byte*)bitmapMemory;
    for (int y = 0; y < bitmapHeight; ++y)
    {
        byte* pixel = (byte*)row;
        for (int x = 0; x < bitmapWidth; ++x)
        {
            //Pixel in memory: 0xBBGGRRxx
            *pixel = (byte)(x + xOffset);
            pixel++;
            *pixel = (byte)(y + yOffset);
            pixel++;
            *pixel = 0;
            pixel++;
            *pixel = 0;
            pixel++;
        }

        row += pitch;
    }
}

//TODO: This is janky af, either refactor or just get rid of lol
void win32_LogInput(int inputIndex)
{
    wchar_t inputLog[32] = {};

    if (inputIndex == 0)
        swprintf_s(inputLog, L"LEFT MOUSE BUTTON");
    else if (inputIndex == 1)
        swprintf_s(inputLog, L"RIGHT MOUSE BUTTON");
    else if (inputIndex == 2)
        swprintf_s(inputLog, L"MIDDLE MOUSE BUTTON");
    else if (inputIndex >= 3 || inputIndex <= 29)
        swprintf_s(inputLog, L"%c KEY", inputIndex - 3 + 'A');
    else if (inputIndex >= 30 || inputIndex <= 40)
        swprintf_s(inputLog, L"%c KEY", inputIndex - 3 + '0');

    wchar_t log[64];
    if (InputDown(input.flags[inputIndex]))
        swprintf_s(log, L"%s DOWN\n", inputLog);
    else if (InputUp(input.flags[inputIndex]))
        swprintf_s(log, L"%s UP\n", inputLog);
    else if (InputHeld(input.flags[inputIndex]))
        swprintf_s(log, L"HOLDING %s\n", inputLog);

    if (input.flags[inputIndex])
        OutputDebugString(log);
}

//DIB: Device Independent Bitmap
internal void win32_ResizeDIB(int width, int height)
{
    if (bitmapMemory)
    {
        VirtualFree(bitmapMemory, 0, MEM_RELEASE);
    }

    bitmapWidth = width;
    bitmapHeight = height;

    bitmapInfo.bmiHeader.biSize        = sizeof(bitmapInfo.bmiHeader);
    bitmapInfo.bmiHeader.biWidth       = bitmapWidth;
    bitmapInfo.bmiHeader.biHeight      = bitmapHeight;
    bitmapInfo.bmiHeader.biPlanes      = 1;
    bitmapInfo.bmiHeader.biBitCount    = 32;
    bitmapInfo.bmiHeader.biCompression = BI_RGB;

    int bitmapMemorySize = PIXEL_IN_BYTES * bitmapWidth * bitmapHeight;
    bitmapMemory = VirtualAlloc(0, bitmapMemorySize, MEM_COMMIT, PAGE_READWRITE);

    Draw(128, 0);    

}

internal void win32_UpdateWindow(HDC deviceContext, RECT* windowRect, 
                                 int x, int y, int width, int height)
{
    int windowWidth = windowRect->right - windowRect->left;
    int windowHeight = windowRect->bottom - windowRect->top;
    StretchDIBits(
        deviceContext, 
        0, 0, bitmapWidth, bitmapHeight,
        0, 0, windowWidth, windowHeight,
        bitmapMemory,
        &bitmapInfo,
        DIB_RGB_COLORS,
        SRCCOPY);
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, 
                    PWSTR pCmdLine, int nCmdShow)
{
    // Register the window class.
    const wchar_t windowName[]  = L"TextEditorWindowClass";
    
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

    ShowWindow(hwnd, nCmdShow);

    // Run the message loop.
    running = true;
    int xOffset = 0;
    int yOffset = 0;
    while (running)
    { 
		win32_LogInput(0);
		win32_LogInput(1);
		win32_LogInput(2);
		for (int i = 0; i < 26; ++i)
			win32_LogInput(3 + i);
		for (int i = 0; i < 10; ++i)
			win32_LogInput(29 + i);

        ProcessInput();

        MSG msg;
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT)
                running = false;

            TranslateMessage(&msg);
            DispatchMessageA(&msg);
        }

        wchar_t inputLog[64];

        Draw(xOffset, yOffset);
        
        HDC hdc = GetDC(hwnd);
        RECT windowRect;
        GetClientRect(hwnd, &windowRect);
        int windowWidth = windowRect.right - windowRect.left;
        int windowHeight = windowRect.bottom - windowRect.top;
        win32_UpdateWindow(hdc, &windowRect, 0, 0, windowWidth, windowHeight);
        ReleaseDC(hwnd, hdc);
        
        ++xOffset;
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
            uint32 c = (uint32)wParam;
            if (c >= (uint32)'A' || c <= (uint32)'Z')
            {
                win32_HandleInputDown(&input.letterKeys[c - 'A']);
            }
            else if (c >= (uint32)'0' || c <= (uint32)'9')
            {
                win32_HandleInputDown(&input.numberKeys[c - '0']);
            }
            else
            {
                //Handle all other keys
            }

        } return 0;

        case WM_SYSKEYUP:
        case WM_KEYUP:
        {
            uint32 c = (uint32)wParam;
            if (c >= (uint32)'A' || c <= (uint32)'Z')
            {
                win32_HandleInputUp(&input.letterKeys[c - 'A']);
            }
            else if (c >= (uint32)'0' || c <= (uint32)'9')
            {
                win32_HandleInputUp(&input.numberKeys[c - '0']);
            }
            else
            {
                //Handle all other keys
            }

        } return 0;
        
        

    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}