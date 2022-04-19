#include "TextEditor_defs.h"
#include "TextEditor_string.h"
#include "TextEditor.h"

string cstring(char* cstr)
{
    string result;
    result.str = cstr;
    result.len = StringLen(cstr);
    return result;
}

char* string::cstring()
{
    char* result = HeapAlloc(char, len + 1);
    memcpy(result, str, len);
    result[len] = 0;
    return result;
}

bool operator==(string lhs, string rhs)
{
    if (lhs.len != rhs.len) return false;

    for (int i = 0; i < lhs.len; ++i)
        if (lhs[i] != rhs[i]) return false;
    
    return true;
}

string SubString(string s, int start, int end)
{
    string result = s;
    result.str += start;
    result.len = ((end == -1) ? s.len : end ) - start;
    return result;
}

bool StringContains(string s, string target)
{
    if (s.len < target.len) return false;

	for (int i = 0; i < s.len; ++i)
	{
		if (s[i] == target[0] && SubString(s, i, i + (int)target.len) == target)
			return true;
	}

    return false;
}

int IndexOfCharInString(string s, char target)
{
    for (int i = 0; i < s.len; ++i)
    {
        if (s[i] == target) return i;
    }
    return -1;
}

int IndexOfLastCharInString(string s, char target)
{
    for (int i = s.len - 1; i >= 0; --i)
    {
        if (s[i] == target) return i;
    }
    return -1;
}

bool CharInString(char c, string s)
{
    for (int i = 0; i < s.len; ++i)
    {
        if (s[i] == c) return true;
    }
    return false;
}

int CharCount(char c, string s)
{
    int result = 0;
    for (int i = 0; i < s.len; ++i)
        result += s[i] == c;
    return result;
}


void string_buf::dealloc()
{
    free(str);
    len = 0;
    cap = 0;   
}

char* string_buf::cstr()
{
    char* result = (char*)malloc(len + 1);
    memcpy(result, str, len);
    result[len] = 0;
    return result;
}

string_buf init_string_buf(string s, size_t capacity)
{ 
    string_buf result = {0, s.len, (capacity) ? capacity : s.len};
    result.str = (char*)malloc(result.cap);
    memcpy(result.str, s.str, s.len);
    return result;
}

void StringBuf_RangeRemove(string_buf* buf, int start, int end)
{
    memcpy(buf->str + start, buf->str + end, end - start);
    buf->len -= end - start;
}

void StringBuf_RemoveStringAt(string_buf* buf, int at, int len)
{
    memcpy(buf->str + at, buf->str + at + len, buf->len - (at + len));
    buf->len -= len;
}

void StringBuf_InsertString(string_buf* buf, string insert, int at)
{
    int prevLen = buf->len;
    buf->len += insert.len;
    ResizeDynamicArray(&buf->str, buf->len, 1, &buf->cap);
    
    int destOffset = buf->len - prevLen + at;
    memmove(buf->str + destOffset, buf->str + at, prevLen - at);
    memcpy(buf->str + at, insert.str, insert.len);
    
    //editor.lines[lineIndex].text[editor.lines[lineIndex].len] = 0;
}

void StringBuf_RemoveAt(string_buf* buf, int at)
{
    buf->len--;
    memcpy(buf->str + at, buf->str + at + 1, buf->len - at);
}


void IntToString(int val, char* buffer)
{
	char* at = buffer;
    int len = 0;
    do 
    {
        *at = (val % 10) + '0';
        val /= 10;
		len++;
        at++;
	} while (val > 0);

    for (int i = 0; i < len / 2; ++i)
    {
        char temp = buffer[len - i - 1];
        buffer[len - i - 1] = buffer[i];
        buffer[i] = temp; 
    }

    buffer[len] = 0;
}

//TODO: Support multiple bases?
int StringToInt(string str, bool* success)
{
    if (str.len == 1 && str[0] == '-')
    {
        if (success) *success = false;
        return -1;
    }

    int result = 0;
    int powOf10 = 1;
    int sign = (str[0] == '-') ? 1 : -1;
    for (int i = str.len - 1; i >= (int)(str[0] == '-'); --i)
    {
        if (i == 0 && str[i] == '-') continue;

        int digit = str[i] - '0';
        if (digit < 0 || digit > 9)
        {
            if (success) *success = false;
            return 0;
        }
            
        result += digit * powOf10;
        powOf10 *= 10;
    }

    return result * sign;
}

//TODO: Overflow check
byte StringToByte(char* str, int len, bool* success)
{
    byte result = 0;
    byte powOf10 = 1;
    for (int i = len - 1; i >= 0; --i)
    {
        byte digit = str[i] - '0';
        if (digit < 0 || digit > 9)
        {
            if (success) *success = false;
            return 0;
        }
            
        result += digit * powOf10;
        powOf10 *= 10;
    }

    return result;
}

char** SplitStringByLines(char* str, int* len)
{
    int numStrings = 1;
    for (char* p = str; p[1]; p++)
    {
        if (p[0] == '\r')
        {
            Assert(p[1] == '\n');
            numStrings++;
            p++;
        }
    }

    char** result = (char**)malloc(numStrings * sizeof(char*));
    char* at = str;
    for (int i = 0; i < numStrings; ++i)
    {
        int lineLen = 0;
        char* start = at;
        for (; at[0] && at[0] != '\r' && at[0] != '\n'; at++)
            lineLen++;

        result[i] = (char*)malloc(lineLen + 1);
        memcpy(result[i], start, lineLen);
        result[i][lineLen] = 0;

        at += (at[0] == '\r') ? 2 : 1;
    }
        
    if (len) *len = numStrings;
    return result;
}

char* AdvanceToNextLine(char* file)
{
    while (file[0] != '\n') 
    {
        if (file[0] == 0)
            return nullptr;
        file++;
    }
    file++;
    return file;
}

char* ReverseString(char* str, int len)
{
    char* result = HeapAlloc(char, len);
    for (int i = 0; i < len; ++i)
        result[len - i - 1] = str[i];
    result[len] = 0;
    return result;
}