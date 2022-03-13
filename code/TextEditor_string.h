#ifndef TEXT_EDITOR_STRING_H
#define TEXT_EDITOR_STRING_H

inline int StringLen(const char* string)
{
    int result = 0;
    for (; string[0]; string++)
        result++;
    return result;
}

inline bool IsForwardsBracket(char c)
{
    return c == '(' || c == '[' || c == '{';
}

inline bool IsBackwardsBracket(char c)
{
    return c == ')' || c == ']' || c == '}';
}

inline bool IsBracket(char c)
{
    return IsForwardsBracket(c) ||IsBackwardsBracket(c);
}

inline char GetOtherBracket(char bracket)
{
    Assert(IsBracket(bracket));

    //idk if there's a clever way to do this but I can't figure it out
    switch(bracket) 
    {
        case '(': return ')';
        case ')': return '(';
        case '[': return ']';
        case ']': return '[';
        case '{': return '}';
        case '}': return '{';
        default: 
        {
            //NOTE: This is unreachable
            Assert(false);
            return 0;
            break;
        }
    }
}

inline bool IsAlphabetical(char c)
{
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
}

inline bool IsNumeric(char c)
{
    return c >= '0' && c <= '9';
}

inline bool IsAlphaNumeric(char c)
{
    return IsAlphabetical(c) || IsNumeric(c);
}

inline bool IsWhiteSpace(char c)
{
    return c <= ' ';
}

//Double check this is correct?
inline bool IsPunctuation(char c)
{
    return !IsAlphaNumeric(c) && !IsWhiteSpace(c);
}

inline bool IsEmptyString(char* str)
{
    return str[0] == 0;
}

void IntToString(int val, char* buffer);
int StringToInt(char* str, int len, bool* success = nullptr);
byte StringToByte(char* str, int len, bool* success = nullptr);
char** SplitStringByLines(char* str, int *len = nullptr);
char* AdvanceToNextLine(char* file);
char* ReverseString(char* str, int len);

#endif