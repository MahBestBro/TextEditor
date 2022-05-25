#include "TextEditor.h"
#include "TextEditor_string.h"
#include "TextEditor_tokeniser.h"

#ifndef TEXT_EDITOR_CONFIG_H
#define TEXT_EDITOR_CONFIG_H

INTROSPECT("GenerateStructMemberOffsets") struct UserSettings
{
    string fontFile;

    Colour backgroundColour;
    Colour lineNumColour;
    Colour cursorColour;
    ColourRGBA highlightColour;
    Colour lineBackgroundColour;
    Colour defaultTextColour;
};

UserSettings LoadUserSettingsFromConfigFile();



#endif