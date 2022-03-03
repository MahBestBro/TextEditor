#include "stdio.h"
#include "stdlib.h"
#include "string.h"

#define __Out

struct Code
{
    int size;
    int len;
    char* buffer;
};

bool CompareStrings(char* str1, int len1, char* str2, int len2)
{
    if (len1 != len2) return false;
    
    for (int i = 0; i < len1; ++i)
    {
        if (str1[i] != str2[i])
            return false;
    }

    return true;
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

int GetDistToCharOrEOS(char* str, char target)
{
    int result = 0;
    while (str[0] && str[0] != target)
    {
        str++;
        result++;
    }
    return result;
}

char* AdvanceToCharAndGetString(char* str, char splitTarget, __Out int* len)
{
    char* result = str;
    int resultLen = 0;
    while (str[0] && str[0] != splitTarget)
    {
        str++;
        resultLen++;
    }
 
    *len = resultLen;
    return result;
}

char* ReadEntireFile(char* fileName)
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

bool WriteToFile(char* fileName, char* text, size_t textLen, int writeStart = 0)
{
    bool result = false;

    FILE* file;
    fopen_s(&file, fileName, "wb");
    if (file)
    {
        if (fseek(file, writeStart, SEEK_SET) == 0)
        {
            result = true;
            fwrite(text, 1, textLen, file);
        }

        fclose(file);
    }

    return result;
}

//TODO: Write function that checks config file for errors.


const char* structStartingCode = 
R"foo(struct UserSettings
{
)foo";
const int structStart = 22;

const char* memberMetaDataStartingCode = 
R"foo(UserSettingsMemberMetaData memberMetaData[] =
{
)foo";
const int memberMetaDataStart = 48;

void AppendStringToCode(Code* code, char* append, int appendLen)
{
    if (code->len + appendLen > code->size)
    {
        code->size *= 2;
        code->buffer = (char*)realloc(code->buffer, code->size);
    }
    memcpy(code->buffer + code->len, append, appendLen);
    code->len += appendLen;
}

void GenerateConfigCode(char* configFile)
{
    Code structCode = {1024, structStart, nullptr};
    structCode.buffer = (char*)malloc(structCode.size);
    memcpy(structCode.buffer, structStartingCode, structStart);

    //TODO: If we start introspecting more structs for metadata, move into another preprocessor
    Code memberMetaDataCode = {1024, memberMetaDataStart, nullptr};
    memberMetaDataCode.buffer = (char*)malloc(memberMetaDataCode.size);
    memcpy(memberMetaDataCode.buffer, memberMetaDataStartingCode, memberMetaDataStart);

    int lineNum = 1;
    char* line = configFile; 
    while (line)
    {
        int lineLen = GetDistToCharOrEOS(line, '\r');

        int varNameLen = GetDistToCharOrEOS(line, ' ');
        char* varName = line;
        
        //Check if variable has value
        if ((varName + varNameLen)[0] != ' ')
        {
            printf("CONFIG CONDE GENERATOR ERROR (line %i): Variable \"%.*s\" needs value.\n", lineNum, varNameLen, varName);
            return;
        }

        int commaCount = 0;
        for (int i = 0; i < lineLen - varNameLen - 1; ++i)
            commaCount += (line + varNameLen + 1)[i] == ',';

        //Check if number of ints correct
        if (commaCount != 0 && commaCount != 2 && commaCount != 3)
        {
            printf("CONFIG CONDE GENERATOR ERROR (line %i): Can only do number groupings of 1, 3 or 4.\n", lineNum);
            return;
        }

        //TODO: Parse string variable

        AppendStringToCode(&structCode, "    ", 4);
        AppendStringToCode(&memberMetaDataCode, "    ", 4);
        if (commaCount == 0)
        {
            AppendStringToCode(&structCode, "int ", 4);
            AppendStringToCode(&structCode, "{TYPE_INT, ", 11);
        }
        else if (commaCount == 2)
        {
            AppendStringToCode(&structCode, "Colour ", 7);
            AppendStringToCode(&memberMetaDataCode, "{TYPE_COLOUR_RGB, ", 18);
        }
        else if (commaCount == 3)
        {
            AppendStringToCode(&structCode, "ColourRGBA ", 11);
            AppendStringToCode(&memberMetaDataCode, "{TYPE_COLOUR_RGBA, ", 19);
        }

        AppendStringToCode(&structCode, varName, varNameLen);
        AppendStringToCode(&structCode, ";\r\n", 3);

        AppendStringToCode(&memberMetaDataCode, "StructOffset(UserSettings, ", 27);
        AppendStringToCode(&memberMetaDataCode, varName, varNameLen);
        if (AdvanceToNextLine(line) != nullptr)
            AppendStringToCode(&memberMetaDataCode, ")},\r\n", 5);
        else
            AppendStringToCode(&memberMetaDataCode, ")}\r\n", 4);

        lineNum++;
        line = AdvanceToNextLine(line);
    }

    AppendStringToCode(&structCode, "};", 2);
    structCode.buffer[structCode.len] = 0;
    AppendStringToCode(&memberMetaDataCode, "};", 2);
    memberMetaDataCode.buffer[memberMetaDataCode.len] = 0;

    WriteToFile("../code/TextEditor_user_settings_struct.inc", 
                structCode.buffer, structCode.len);
    WriteToFile("../code/TextEditor_user_settings_memberData.inc", 
                memberMetaDataCode.buffer, memberMetaDataCode.len);
}

int main()
{
    char* configFileName = "../config/config_general.txt";
    
    char* configFile = ReadEntireFile(configFileName);
    if (configFile)
    {
        GenerateConfigCode(configFile);
    }
    else printf("CONFIG CODE GENERATOR ERROR: Could not find file \"%s\"\n", configFileName);


    return 0;
}