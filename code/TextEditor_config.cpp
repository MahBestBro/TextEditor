#include "TextEditor_defs.h"
#include "TextEditor.h"
#include "TextEditor_string.h"
#include "TextEditor_meta.h"
#include "TextEditor_config.h"

char* AdvanceOnePastChar(char* str, char target, __Out int* len = 0)
{
    int resultLen = 0;
    while (str[0] && str[0] != target)
    {
        str++;
        resultLen++;
    }
    if (str[0]) str++;
 
    if (len) *len = resultLen;
    return str;
}

char* AdvanceToCharAndGetString(char* str, char target, __Out int* len)
{
    char* result = str;
    int resultLen = 0;
    while (str[0] && str[0] != target)
    {
        str++;
        resultLen++;
    }
 
    *len = resultLen;
    return result;
}

UserSettings LoadUserSettingsFromConfigFile()
{
    UserSettings result = {};

    char* file = ReadEntireFileAsCstr("config/config_general.txt");
    Assert(file);

    int numLines = 0;
    char** lines = SplitStringByLines(file, &numLines);
    for (int i = 0; i < numLines; ++i)
    {
        MemberMetaData memberData = UserSettingsMemberMetaData[i];
        char* val = AdvanceOnePastChar(lines[i], ' ');

        switch(memberData.type)
        {
            case TYPE_Colour:
            {
                Colour colour;

                int offset = 0;
                bool success = true;

                char* rStr = AdvanceToCharAndGetString(val, ',', &offset);
                byte r = StringToByte(rStr, offset, &success);
                Assert(success);
                colour.r = r;

                char* gStr = AdvanceToCharAndGetString(rStr + offset + 1, ',', &offset);
                byte g = StringToByte(gStr, offset, &success);
                Assert(success);
                colour.g = g;

                char* bStr = AdvanceToCharAndGetString(gStr + offset + 1, ',', &offset);
                byte b = StringToByte(bStr, offset, &success);
                Assert(success);
                colour.b = b;

                *(Colour*)MemberPtr(result, memberData.offset) = colour;
            } break;

            case TYPE_ColourRGBA:
            {
                ColourRGBA colour;

                int offset = 0;
                bool success = true;

                char* rStr = AdvanceToCharAndGetString(val, ',', &offset);
                byte r = StringToByte(rStr, offset, &success);
                Assert(success);
                colour.r = r;

                char* gStr = AdvanceToCharAndGetString(rStr + offset + 1, ',', &offset);
                byte g = StringToByte(gStr, offset, &success);
                Assert(success);
                colour.g = g;

                char* bStr = AdvanceToCharAndGetString(gStr + offset + 1, ',', &offset);
                byte b = StringToByte(bStr, offset, &success);
                Assert(success);
                colour.b = b;

                char* aStr = AdvanceToCharAndGetString(bStr + offset + 1, ',', &offset);
                byte a = StringToByte(aStr, offset, &success);
                Assert(success);
                colour.a = a;

                *(ColourRGBA*)MemberPtr(result, memberData.offset) = colour;
            } break;

            case TYPE_int:
            {
                UNIMPLEMENTED("TODO: Implement once needed")
            } break;

            case TYPE_bool:
            {
                UNIMPLEMENTED("TODO: Implement once needed")
            } break;
        }
    }

    for (int i = 0; i < numLines; ++i)
        free(lines[i]);
    free(lines);

    FreeWin32(file);

    return result;
}