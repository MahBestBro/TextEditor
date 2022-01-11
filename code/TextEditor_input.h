#include "TextEditor_defs.h"

#ifndef TEXT_EDITOR_INPUT_H
#define TEXT_EDITOR_INPUT_H

#define INPUT_DOWN 0b001
#define INPUT_UP   0b010
#define INPUT_HELD 0b100

enum InputCode
{
    INPUTCODE_LEFT_MOUSE,
    INPUTCODE_RIGHT_MOUSE,
    INPUTCODE_MIDDLE_MOUSE,
    
    INPUTCODE_LEFT,
    INPUTCODE_RIGHT,
    INPUTCODE_UP,
    INPUTCODE_DOWN,

    INPUTCODE_BACKSPACE,
    INPUTCODE_LSHIFT,
    INPUTCODE_LCTRL,
    INPUTCODE_LALT,
    INPUTCODE_CAPSLOCK,
    INPUTCODE_ENTER,

    INPUTCODE_SPACE,
    INPUTCODE_TAB,

    INPUTCODE_A,
    INPUTCODE_B,
    INPUTCODE_C,
    INPUTCODE_D,
    INPUTCODE_E,
    INPUTCODE_F,
    INPUTCODE_G,
    INPUTCODE_H,
    INPUTCODE_I,
    INPUTCODE_J,
    INPUTCODE_K,
    INPUTCODE_L,
    INPUTCODE_M,
    INPUTCODE_N,
    INPUTCODE_O,
    INPUTCODE_P,
    INPUTCODE_Q,
    INPUTCODE_R,
    INPUTCODE_S,
    INPUTCODE_T,
    INPUTCODE_U,
    INPUTCODE_V,
    INPUTCODE_W,
    INPUTCODE_X,
    INPUTCODE_Y,
    INPUTCODE_Z,

    INPUTCODE_0,
    INPUTCODE_1,
    INPUTCODE_2,
    INPUTCODE_3,
    INPUTCODE_4,
    INPUTCODE_5,
    INPUTCODE_6,
    INPUTCODE_7,
    INPUTCODE_8,
    INPUTCODE_9,

    INPUTCODE_OPEN_SQ_BRACKET,
    INPUTCODE_CLOSED_SQ_BRACKET,
    INPUTCODE_SEMICOLON,
    INPUTCODE_QUOTE,
    INPUTCODE_COMMA,
    INPUTCODE_PERIOD,
    INPUTCODE_FORWARD_SLASH,

    INPUTCODE_BACK_SLASH,
    INPUTCODE_MINUS,
    INPUTCODE_EQUALS,
    INPUTCODE_GRAVE,

    NUM_INPUTS
};

#define MOUSE_START INPUTCODE_LEFT_MOUSE
#define KEYS_START INPUTCODE_LEFT
#define ARROWS_START INPUTCODE_LEFT
#define CHAR_KEYS_START INPUTCODE_SPACE
#define NON_SHIFT_START INPUTCODE_BACKSPACE
#define SHIFT_START INPUTCODE_A
#define LETTER_START INPUTCODE_A
#define NUMBER_START INPUTCODE_0
#define PUNCTUATION_START INPUTCODE_OPEN_SQ_BRACKET
#define OTHER_SYMBOLS_START INPUTCODE_BACK_SLASH

union Input
{
    struct
    {
        byte leftMouse;
        byte rightMouse;
        byte middleMouse;

        union
        {
            struct
            {
                byte left;
                byte up;
                byte right;
                byte down;
            };
            byte arrowKeys[4];
        };  

        byte backspace;
        byte leftShift;
        byte leftCtrl;
        byte leftAlt;
        byte capsLock;
        byte enter;

        byte space;
        byte tab;

        byte letterKeys[26];
        byte numberKeys[10];
        byte punctuation[7];
        byte backslash;
        byte minus;
        byte equals;
        byte grave;
        
    };
    byte flags[NUM_INPUTS];
};

char* InputCodeToStr(InputCode code);
char InputCodeToChar(InputCode code, bool shift, bool caps);

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

inline bool ArrowKeysHeld(byte* arrowKeyFlags)
{
    bool result = false;
    for (int i = 0; i < 4; ++i)
    {
        if (InputHeld(arrowKeyFlags[i]))
        {
            result = true;
            break;
        }
    }
    return result;
}

#endif
