#include "stdio.h"
#include "stdlib.h"
#include "string.h"

struct string_buf
{
    char* str;
    size_t len;
    size_t cap;
};

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

bool CompareStrings(char* a, int aLen, char* b, int bLen)
{
    if (aLen != bLen) return false;

    for (int i = 0; i < aLen; ++i)
    {
        if (a[i] != b[i]) return false;
    }
    return true;
}

char* ReadEntireFile(const char* fileName)
{
    char* result = nullptr;

    FILE* file;
    fopen_s(&file, fileName, "rb");
    if (file)
    {
        fseek(file, 0, SEEK_END);
        int len = ftell(file);
        fseek(file, 0, SEEK_SET);

        result = (char*)malloc(len+1);
        fread(result, 1, len, file);
        result[len] = 0;

        fclose(file);
    }

    return result;
}

bool WriteEntireFile(char* fileName, char* text, size_t textLen)
{
    bool result = false;

    FILE* file;
    fopen_s(&file, fileName, "wb");
    if (file)
    {
        if (fseek(file, 0, SEEK_SET) == 0)
        {
            result = true;
            fwrite(text, 1, textLen, file);
        }

        fclose(file);
    }

    return result;
}

char* SkipOverWhitespace(char* str)
{
    while(str[0] && str[0] <= ' ') str++;
    return str;
}

char* SkipOverWord(char* str)
{
	while (str[0] && str[0] > ' ') str++;
	return str;
}

char* AdvanceToChar(char* str, char target)
{
    while (str[0] && str[0] != target) str++;
    return str;
}

char* AdvanceToAnyOneChar(char* str, char* targets, int numTargets)
{
    while (str[0])
    {
        for (int i = 0; i < numTargets; ++i)
        {
            if (targets[i] == str[0]) return str;
        }
        str++;
    }
    return str;
}

char* AdvanceToNextLine(char* file)
{
	while (file[0] && file[0] != '\n') file++;
    file += file[0] == '\n';
    return file;
}

void AppendToStringBuffer(string_buf* strbuf, char* str, size_t strLen)
{
    strbuf->len += strLen;
    
    bool shouldRealloc = false;
    while (strbuf->len > strbuf->cap) 
    {
        shouldRealloc = true;
        strbuf->cap *= 2;
    }
    if (shouldRealloc) strbuf->str = (char*)realloc(strbuf->str, strbuf->cap);

    memcpy(strbuf->str + strbuf->len - strLen, str, strLen);
}




void GenerateStructMemberOffsets(char* code)
{
    string_buf generatedCode = {0}; 
    generatedCode.str = (char*)malloc(128);
    generatedCode.cap = 128;

    code = SkipOverWhitespace(code);

    if (!CompareStrings(code, 6, "struct", 6) && !CompareStrings(code, 5, "union", 5))
    {
        //TODO: Log which file (and even line maybe) it was in as well
        printf("INTROSPECT ERROR!: Expected struct or union for GenerateStructMemberOffsets and did not get either.\n");
        return;
    }

    code = SkipOverWord(code);
    code = SkipOverWhitespace(code);

    char* typeName = code;
    code = SkipOverWord(code);
    size_t typeLen = code - typeName;

    AppendToStringBuffer(&generatedCode, "MemberMetaData ", 15);
    AppendToStringBuffer(&generatedCode, typeName, typeLen);
    AppendToStringBuffer(&generatedCode, "MemberMetaData[] =\r\n{\r\n", 23);
    //printf("Struct: %.*s\n", (int)typeLen, typeName);
    
    code = AdvanceToChar(code, '{');
    int scopeDepth = 0;
	bool parsing = true;
    //code = AdvanceToAnyOneChar(code, ',;', 2);
    for (; code[0] && parsing; code++)
    {
        //TODO: Check for case when array member initialised 
        switch (code[0])
        {
            case '=':
            case ';':
            {
				if (code[0] == ';' && (code - 1)[0] == '}') break;

                //Get Member Name
                char* memberNameEnd = code - 1;
                while(memberNameEnd[0] <= ' ') memberNameEnd--;

                if (memberNameEnd[0] == ']')
                {
                    while (memberNameEnd[0] != '[') memberNameEnd--;
					memberNameEnd--;
                }
                char* memberName = memberNameEnd;
                while(IsAlphaNumeric(memberName[0])) memberName--;
				memberName++;
                size_t memberNameLen = memberNameEnd - memberName + 1;

                //printf("Name: %.*s, ", (int)memberNameLen, memberName);

                //We might not need this???
                if ((memberName - 1)[0] > ' ')
                {
                    printf("INTROSPECT ERROR!: Invalid char '%c' in member name.\n", (memberName - 1)[0]);
                    return;
                }

                //Get Member Type
                char* memberTypeEnd = memberName - 1;
                while (memberTypeEnd[0] <= ' ') memberTypeEnd--;
				memberTypeEnd++;
				char* memberType = memberTypeEnd - 1;
				//TODO: Include '<', '>' and ':' in loop
				while (IsAlphaNumeric(memberType[0])) memberType--;
                memberType++;
                size_t memberTypeLen = memberTypeEnd - memberType;


                AppendToStringBuffer(&generatedCode, "    {TYPE_", 10);
                AppendToStringBuffer(&generatedCode, memberType, memberTypeLen);
                AppendToStringBuffer(&generatedCode, ", StructOffset(", 15);
                AppendToStringBuffer(&generatedCode, typeName, typeLen);
                AppendToStringBuffer(&generatedCode, ", ", 2);
                AppendToStringBuffer(&generatedCode, memberName, memberNameLen);
                AppendToStringBuffer(&generatedCode, ")},\r\n", 5);

                //printf("Type: %.*s\n", (int)memberTypeLen, memberType);

                code = AdvanceToNextLine(code);
            } break;

            case '{': scopeDepth++; break;
            case '}': scopeDepth--; break;
        }

		parsing = scopeDepth > 0;
    }

    AppendToStringBuffer(&generatedCode, "};", 2);
    //printf("%s\n", generatedCode.str);
    WriteEntireFile("code/TextEditor_meta.cpp", generatedCode.str, generatedCode.len);
}

char* introspectArgs[] =
{
    "\"GenerateStructMemberOffsets\"",
    ""
};

void (*introspectFuncs[])(char*) =
{
    GenerateStructMemberOffsets
};

char* fileNames[] = 
{
    "code/TextEditor_config.h",
    ""
};

int main()
{
    for (int f = 0; fileNames[f][0]; ++f)
    {
        char* file = ReadEntireFile(fileNames[f]);
        while(file[0])
        {
            file = SkipOverWhitespace(file);

            if (CompareStrings(file, 11, "INTROSPECT(", 11))
            {
                file += 11;
                for (int i = 0; introspectArgs[i][0]; ++i)
                {
                    int argLen = (int)strlen(introspectArgs[i]);
                    if (CompareStrings(file, argLen, introspectArgs[i], argLen))
                    {
                        file += argLen + 1; //+1 to skip over ')'
                        introspectFuncs[i](file);
                    }
                }
            }
            
            file = AdvanceToNextLine(file);
        }
    }

    return 0;
}