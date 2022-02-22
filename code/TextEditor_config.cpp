#include "TextEditor_defs.h"
#include "TextEditor.h"
#include "TextEditor_string.h"
#include "TextEditor_config.h"

UserSettings LoadUserSettingsFromConfigFile()
{
    UserSettings result;

    char* file = ReadEntireFile("config/config_general.txt");

    char* line = AdvanceToNextLine(file);
    while (file != nullptr)
    {
        int nameLen = 0;
        char* name = SplitString(line, ' ', &lineLen);

        if (CompareStrings(name, nameLen, "backgroundColour")) 



        line = AdvanceToNextLine(line);
    }

    return result;
}