//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#include "inspector_object.h"

#include "../../../src/editor/editor_messages.h"

void InspectorObject::AddProperty(const std::string& name, const std::string& value)
{
    _properties.AddProperty(name, value);
}

void InspectorObject::AddProperty(const std::string& name, const TString& value)
{
    _properties.AddProperty(name, value);
}

const ObjectProperty* InspectorObject::FindProperty(const std::string& name) const
{
    for (const auto& prop : _properties.GetProperties())
        if (prop.name == name)
            return &prop;

    return nullptr;
}

std::unique_ptr<InspectorObject> InspectorObject::CreateFromStream(Stream* stream)
{
    std::vector<InspectorObject*> stack;

    while (!IsEOS(stream))
    {
        switch (ReadU8(stream))
        {
        case INSPECTOR_OBJECT_COMMAND_BEGIN:
        {
            char name_buffer[1024];
            ReadString(stream, name_buffer, 1024);
            auto object = new InspectorObject(name_buffer);
            stack.push_back(object);
            break;
        }

        case INSPECTOR_OBJECT_COMMAND_END:
        {
            if (stack.size() == 1)
                return std::unique_ptr<InspectorObject>(stack.front());

            auto object = stack.back();
            stack.pop_back();
            stack.back()->AddChild(std::unique_ptr<InspectorObject>(object));
            break;
        }

        case INSPECTOR_OBJECT_COMMAND_PROPERTY:
        {
            auto object = stack.back();

            char name_buffer[1024];
            ReadString(stream, name_buffer, 1024);
            switch (ReadU8(stream))
            {
            case INSPECTOR_OBJECT_COMMAND_VEC3:
                object->AddProperty(name_buffer, TStringBuilder().Add(ReadVec3(stream)).ToString());
                break;
            }
            break;
        }

        default:
            break;
        }
    }

    return nullptr;
}
