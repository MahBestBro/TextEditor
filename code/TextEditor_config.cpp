#include "TextEditor_defs.h"
#include "TextEditor.h"
#include "TextEditor_string.h"
#include "TextEditor_meta.h"
#include "TextEditor_config.h"

UserSettings LoadUserSettingsFromConfigFile()
{
    UserSettings result = {};

    string file = ReadEntireFileAsString(lstring("config/config_general.txt"));
    Assert(file.str);

    string line = GetNextLine(&file);
    for (int i = 0; line[0]; ++i)
    {
        MemberMetaData memberData = UserSettingsMemberMetaData[i];
        string val = SubString(line, IndexOfCharInString(line, ' ') + 1);

        switch(memberData.type)
        {
            case TYPE_Colour:
            {
                Colour colour;

                //int offset = 0;
                bool success = true;

                byte r = StringToByte(AdvanceToCharAndSplitString(&val, ','), &success);
                Assert(success);
                colour.r = r;

                byte g = StringToByte(AdvanceToCharAndSplitString(&val, ','), &success);
                Assert(success);
                colour.g = g;

                byte b = StringToByte(AdvanceToCharAndSplitString(&val, ','), &success);
                Assert(success);
                colour.b = b;

                *(Colour*)MemberPtr(result, memberData.offset) = colour;
            } break;

            case TYPE_ColourRGBA:
            {
                ColourRGBA colour;

                bool success = true;

                byte r = StringToByte(AdvanceToCharAndSplitString(&val, ','), &success);
                Assert(success);
                colour.r = r;

                byte g = StringToByte(AdvanceToCharAndSplitString(&val, ','), &success);
                Assert(success);
                colour.g = g;

                byte b = StringToByte(AdvanceToCharAndSplitString(&val, ','), &success);
                Assert(success);
                colour.b = b;

                byte a = StringToByte(AdvanceToCharAndSplitString(&val, ','), &success);
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

        line = GetNextLine(&file);
    }

    FreeWin32(file.str);

    return result;
}