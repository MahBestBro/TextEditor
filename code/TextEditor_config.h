#include "TextEditor_tokeniser.h"

#ifndef TEXT_EDITOR_CONFIG_H
#define TEXT_EDITOR_CONFIG_H

INTROSPECT("GenerateStructMemberOffsets") struct UserSettings
{
    Colour backgroundColour;
    Colour lineNumColour;
    Colour cursorColour;
    ColourRGBA highlightColour;
    Colour defaultTextColour;
    
    union 
    {
        struct
        {
            Colour punctuationColour;
            Colour operatorColour;
            Colour stringColour;
            Colour numberColour;
            Colour boolColour;
            Colour identifierColour;
            Colour functionColour;
            Colour customTypeColour;
            Colour inbuiltTypeColour;
            Colour keywordColour;
            Colour preprocessorColour;
            Colour defineColour;
            Colour commentColour;
            Colour unknownColour;
        };
        Colour tokenColours[NUM_TOKENS - 1]; //excluding TOKEN_EOS 
    };
};

UserSettings LoadUserSettingsFromConfigFile();

#endif