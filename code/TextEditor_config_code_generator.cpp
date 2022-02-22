#include "stdio.h"
#include "stdlib.h"
#include "string.h"

#define __Out

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
const char* structEndingCode = R"foo(};)foo";
const int structEndLen = 2;

//const char* memberMetaDataStartingCode = 
//R"foo(UserSettingsMemberMetaData memberMetaData[] =
//{
//)foo";
//const int memberMetaDataStart = 50;

void GenerateConfigCode(char* configFile)
{
    int structCodeSize = 1024;
    int structCodeLen = structStart;
    char* structCode = (char*)malloc(structCodeSize);
    memcpy(structCode, structStartingCode, structStart);

    //TODO: If we start introspecting more structs for metadata, move into another preprocessor
    //int memberMetaDataCodeSize = 1024;
    //int memberMetaDataCodeLen = structStart;
    //char* memberMetaDataCode = (char*)malloc(structCodeSize);
    //memcpy(structCode, memberMetaDataStartingCode, memberMetaDataStart);

    int lineNum = 1;
    char* line = configFile; 
    while (line)
    {
        int lineLen = GetDistToCharOrEOS(line, '\r');

        if (structCodeLen + lineLen > structCodeSize)
        {
            structCodeSize *= 2;
            structCode = (char*)realloc(structCode, structCodeSize);
        }

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

        memcpy(structCode + structCodeLen, "    ", 4);
        structCodeLen += 4;
        if (commaCount == 0)
        {
            memcpy(structCode + structCodeLen, "int ", 4);
            structCodeLen += 4;
        }
        else if (commaCount == 2)
        {
            memcpy(structCode + structCodeLen, "Colour ", 7);
            structCodeLen += 7;
        }
        else if (commaCount == 3)
        {
            memcpy(structCode + structCodeLen, "ColourRGBA ", 11);
            structCodeLen += 11;
        }

        memcpy(structCode + structCodeLen, varName, varNameLen);
        structCodeLen += varNameLen;
        memcpy(structCode + structCodeLen, ";\r\n", 3);
        structCodeLen += 3;

        lineNum++;
        line = AdvanceToNextLine(line);
    }

    if (structCodeLen + structEndLen > structCodeSize)
    {
        structCodeSize *= 2;
        structCode = (char*)realloc(structCode, structCodeSize);
    }
    memcpy(structCode + structCodeLen, structEndingCode, structEndLen);
    structCodeLen += structEndLen;
    structCode[structCodeLen] = 0;

    WriteToFile("../code/TextEditor_user_settings.inc", structCode, structCodeLen);
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