#ifndef TEXT_EDITOR_META_H
#define TEXT_EDITOR_META_H

#define StructOffset(structType, member) (uint64)&((structType*)0)->member
#define MemberPtr(structInstance, offset) (((byte*)&structInstance) + offset)

enum UserSettingsType
{
    TYPE_COLOUR_RGB,
    TYPE_COLOUR_RGBA,
    TYPE_INT,
    TYPE_BOOL,
    TYPE_STRING
};

struct UserSettingsMemberMetaData
{
    UserSettingsType type;
    uint64 offset;
};

#endif