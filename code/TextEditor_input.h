#include "TextEditor.h"

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

    INPUTCODE_SPACE,
    INPUTCODE_BACKSPACE,
    INPUTCODE_ENTER,
    INPUTCODE_LSHIFT,
    INPUTCODE_TAB,
    INPUTCODE_LCTRL,
    INPUTCODE_LALT,
    INPUTCODE_CAPSLOCK,

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
#define ARROWS_START INPUTCODE_UP
#define NON_SHIFT_START INPUTCODE_SPACE
#define SHIFT_START INPUTCODE_A
#define LETTER_START INPUTCODE_A
#define NUMBER_START INPUTCODE_0
#define PUNCTUATION_START INPUTCODE_OPEN_SQ_BRACKET
#define OTHER_SYMBOLS_START INPUTCODE_BACKSLASH

inline char* InputCodeToStr(InputCode code)
{
    switch (code)
    {
        case INPUTCODE_LEFT_MOUSE:  return "LEFT MOUSE";
        case INPUTCODE_RIGHT_MOUSE:  return "RIGHT MOUSE";
        case INPUTCODE_MIDDLE_MOUSE: return "MIDDLE MOUSE";
    
        case INPUTCODE_LEFT:  return "LEFT";
        case INPUTCODE_RIGHT: return "RIGHT";
        case INPUTCODE_UP:    return "UP";
        case INPUTCODE_DOWN:  return "DOWN";

        case INPUTCODE_SPACE:     return "SPACE";
        case INPUTCODE_BACKSPACE: return "BACKSPACE";
        case INPUTCODE_ENTER:     return "ENTER";
        case INPUTCODE_LSHIFT:    return "LSHIFT";
        case INPUTCODE_TAB:       return "TAB";
        case INPUTCODE_LCTRL:     return "LCTRL";
        case INPUTCODE_LALT:      return "LALT";
        case INPUTCODE_CAPSLOCK:  return "CAPSLOCK";

        case INPUTCODE_A: return "A";
        case INPUTCODE_B: return "B";
        case INPUTCODE_C: return "C";
        case INPUTCODE_D: return "D";
        case INPUTCODE_E: return "E";
        case INPUTCODE_F: return "F";
        case INPUTCODE_G: return "G";
        case INPUTCODE_H: return "H";
        case INPUTCODE_I: return "I";
        case INPUTCODE_J: return "J";
        case INPUTCODE_K: return "K";
        case INPUTCODE_L: return "L";
        case INPUTCODE_M: return "M";
        case INPUTCODE_N: return "N";
        case INPUTCODE_O: return "O";
        case INPUTCODE_P: return "P";
        case INPUTCODE_Q: return "Q";
        case INPUTCODE_R: return "R";
        case INPUTCODE_S: return "S";
        case INPUTCODE_T: return "T";
        case INPUTCODE_U: return "U";
        case INPUTCODE_V: return "V";
        case INPUTCODE_W: return "W";
        case INPUTCODE_X: return "X";
        case INPUTCODE_Y: return "Y";
        case INPUTCODE_Z: return "Z";

        case INPUTCODE_0: return "0";
        case INPUTCODE_1: return "1";
        case INPUTCODE_2: return "2";
        case INPUTCODE_3: return "3";
        case INPUTCODE_4: return "4";
        case INPUTCODE_5: return "5";
        case INPUTCODE_6: return "6";
        case INPUTCODE_7: return "7";
        case INPUTCODE_8: return "8";
        case INPUTCODE_9: return "9";

        case INPUTCODE_OPEN_SQ_BRACKET:   return "OPEN SQ BRACKET";
        case INPUTCODE_CLOSED_SQ_BRACKET: return "CLOSED SQ BRACKET";
        case INPUTCODE_SEMICOLON:         return "SEMICOLON";
        case INPUTCODE_QUOTE:             return "QUOTE";
        case INPUTCODE_COMMA:             return "COMMA";
        case INPUTCODE_PERIOD:            return "PERIOD";
        case INPUTCODE_FORWARD_SLASH:     return "FORWARD SLASH";
        
        case INPUTCODE_BACK_SLASH: return "BACK SLASH";
        case INPUTCODE_MINUS:      return "MINUS";
        case INPUTCODE_EQUALS:     return "EQUALS";
        case INPUTCODE_GRAVE:      return "GRAVE";

        default: return "NULL";
    }
}

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

        byte space;
        byte backspace;
        byte enter;
        byte leftShift;
        byte tab;
        byte leftCtrl;
        byte leftAlt;
        byte capsLock;

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

#endif