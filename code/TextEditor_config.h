#include "TextEditor_defs.h"
#include "TextEditor.h"
#include "TextEditor_string.h"

#define StructOffset(struct, member) (uint32)&((struct*)0)->member

struct UserSettingsType
{
    COLOUR_RGB,
    COLOUR_RGBA,
    INT,
    BOOL,
    STRING
};

struct UserSettingsMemberMetaData
{
    UserSettingsType type;
    uint32 offset;
};

UserSettingsMemberMetaData memberMetaData[] = 
{
    {COLOUR_RGB, StructOffset(UserSettings, member)},
};

#include "TextEditor_user_settings.inc"

UserSettings LoadUserSettingsFromConfigFile();