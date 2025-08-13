/*

	NoZ Game Engine

	Copyright(c) 2025 NoZ Games, LLC

*/

#include <noz/StreamReader.h>
#include <noz/StreamWriter.h>

namespace noz::ui
{
    static Style defaultStyle()
    {
        Style style;
        style.flexDirection = FlexDirection::Row;
        style.width = StyleLength::Auto;
        style.height = StyleLength::Auto;
        style.backgroundColor = Color::Transparent;
        style.color = Color::White;
        style.fontSize = 16.0f;
        style.borderRadius = 0.0f;
        style.borderWidth = 0.0f;
        style.borderColor = Color::Transparent;
        style.font = 0;
        style.imageTint = Color::White;
        style.imageScaleMode = ScaleMode::ScaleToFit;
        style.imageFlip = false;
        style.textAlignment = TextAlignment::TopLeft;
        style.textOutlineWidth = 0.0f;
        style.textOutlineColor = Color::Transparent;
        style.marginTop = StyleLength::fixed(0.0f);
        style.marginLeft = StyleLength::fixed(0.0f);
        style.marginBottom = StyleLength::fixed(0.0f);
        style.marginRight = StyleLength::fixed(0.0f);
        style.paddingTop = StyleLength::fixed(0.0f);
        style.paddingLeft = StyleLength::fixed(0.0f);
        style.paddingBottom = StyleLength::fixed(0.0f);
        style.paddingRight = StyleLength::fixed(0.0f);
        
        // Reset all style parameters to Inherit for the default style
        // This ensures that the default style doesn't force overwrite behavior
        style.flexDirection.keyword = StyleKeyword::Inherit;
        style.width.keyword = StyleKeyword::Inherit;
        style.height.keyword = StyleKeyword::Inherit;
        style.backgroundColor.keyword = StyleKeyword::Inherit;
        style.color.keyword = StyleKeyword::Inherit;
        style.fontSize.keyword = StyleKeyword::Inherit;
        style.borderRadius.keyword = StyleKeyword::Inherit;
        style.borderWidth.keyword = StyleKeyword::Inherit;
        style.borderColor.keyword = StyleKeyword::Inherit;
        style.font.keyword = StyleKeyword::Inherit;
        style.imageTint.keyword = StyleKeyword::Inherit;
        style.imageScaleMode.keyword = StyleKeyword::Inherit;
        style.imageFlip.keyword = StyleKeyword::Inherit;
        style.textAlignment.keyword = StyleKeyword::Inherit;
        style.textOutlineWidth.keyword = StyleKeyword::Inherit;
        style.textOutlineColor.keyword = StyleKeyword::Inherit;
        style.marginTop.keyword = StyleKeyword::Inherit;
        style.marginLeft.keyword = StyleKeyword::Inherit;
        style.marginBottom.keyword = StyleKeyword::Inherit;
        style.marginRight.keyword = StyleKeyword::Inherit;
        style.paddingTop.keyword = StyleKeyword::Inherit;
        style.paddingLeft.keyword = StyleKeyword::Inherit;
        style.paddingBottom.keyword = StyleKeyword::Inherit;
        style.paddingRight.keyword = StyleKeyword::Inherit;
        
        return style;
    }

    Style Style::s_default = defaultStyle();

    void Style::apply(const Style* style)
    {
        if (!style)
            return;

        apply(*style);
    }

    void Style::apply(const Style& style)
    {
        // Apply all style properties that have Overwrite keyword
        if (style.flexDirection.isOverwrite())
            flexDirection = style.flexDirection;
        if (style.width.isOverwrite())
            width = style.width;
        if (style.height.isOverwrite())
            height = style.height;
        if (style.backgroundColor.isOverwrite())
            backgroundColor = style.backgroundColor;
        if (style.color.isOverwrite())
            color = style.color;
        if (style.fontSize.isOverwrite())
            fontSize = style.fontSize;
        if (style.borderRadius.isOverwrite())
            borderRadius = style.borderRadius;
        if (style.borderWidth.isOverwrite())
            borderWidth = style.borderWidth;
        if (style.borderColor.isOverwrite())
            borderColor = style.borderColor;
        if (style.font.isOverwrite())
            font = style.font;
        if (style.imageTint.isOverwrite())
            imageTint = style.imageTint;
        if (style.imageScaleMode.isOverwrite())
            imageScaleMode = style.imageScaleMode;
        if (style.imageFlip.isOverwrite())
            imageFlip = style.imageFlip;
        if (style.textAlignment.isOverwrite())
            textAlignment = style.textAlignment;
        if (style.textOutlineWidth.isOverwrite())
            textOutlineWidth = style.textOutlineWidth;
        if (style.textOutlineColor.isOverwrite())
            textOutlineColor = style.textOutlineColor;
        if (style.marginTop.isOverwrite())
            marginTop = style.marginTop;
        if (style.marginLeft.isOverwrite())
            marginLeft = style.marginLeft;
        if (style.marginBottom.isOverwrite())
            marginBottom = style.marginBottom;
        if (style.marginRight.isOverwrite())
            marginRight = style.marginRight;
        if (style.paddingTop.isOverwrite())
            paddingTop = style.paddingTop;
        if (style.paddingLeft.isOverwrite())
            paddingLeft = style.paddingLeft;
        if (style.paddingBottom.isOverwrite())
            paddingBottom = style.paddingBottom;
        if (style.paddingRight.isOverwrite())
            paddingRight = style.paddingRight;
    }


    void Style::serialize(StreamWriter& writer) const
    {
        flexDirection.serialize(writer);
        width.serialize(writer);
        height.serialize(writer);
        backgroundColor.serialize(writer);
        color.serialize(writer);
        fontSize.serialize(writer);
        borderRadius.serialize(writer);
        borderWidth.serialize(writer);
        borderColor.serialize(writer);
        font.serialize(writer);
        imageTint.serialize(writer);
        imageScaleMode.serialize(writer);
        imageFlip.serialize(writer);
        textAlignment.serialize(writer);
        textOutlineWidth.serialize(writer);
        textOutlineColor.serialize(writer);
        marginTop.serialize(writer);
        marginLeft.serialize(writer);
        marginBottom.serialize(writer);
        marginRight.serialize(writer);
        paddingTop.serialize(writer);
        paddingLeft.serialize(writer);
        paddingBottom.serialize(writer);
        paddingRight.serialize(writer);
    }

    void Style::deserialize(StreamReader& reader)
    {
        flexDirection.deserialize(reader);
        width.deserialize(reader);
        height.deserialize(reader);
        backgroundColor.deserialize(reader);
        color.deserialize(reader);
        fontSize.deserialize(reader);
        borderRadius.deserialize(reader);
        borderWidth.deserialize(reader);
        borderColor.deserialize(reader);
        font.deserialize(reader);
        imageTint.deserialize(reader);
        imageScaleMode.deserialize(reader);
        imageFlip.deserialize(reader);
        textAlignment.deserialize(reader);
        textOutlineWidth.deserialize(reader);
        textOutlineColor.deserialize(reader);
        marginTop.deserialize(reader);
        marginLeft.deserialize(reader);
        marginBottom.deserialize(reader);
        marginRight.deserialize(reader);
        paddingTop.deserialize(reader);
        paddingLeft.deserialize(reader);
        paddingBottom.deserialize(reader);
        paddingRight.deserialize(reader);
    }
    
    void StyleColor::serialize(StreamWriter& writer) const
    {
        writer.write(static_cast<uint8_t>(keyword));
        writer.write(value.r);
        writer.write(value.g);
        writer.write(value.b);
        writer.write(value.a);
    }
    
    void StyleColor::deserialize(StreamReader& reader)
    {
        keyword = static_cast<StyleKeyword>(reader.read<uint8_t>());
        value.r = reader.read<float>();
        value.g = reader.read<float>();
        value.b = reader.read<float>();
        value.a = reader.read<float>();
    }
    
    void StyleFloat::serialize(StreamWriter& writer) const
    {
        writer.write(static_cast<uint8_t>(keyword));
        writer.write(value);
    }
    
    void StyleFloat::deserialize(StreamReader& reader)
    {
        keyword = static_cast<StyleKeyword>(reader.read<uint8_t>());
        value = reader.read<float>();
    }
    
    void StyleInt::serialize(StreamWriter& writer) const
    {
        writer.write(static_cast<uint8_t>(keyword));
        writer.write(value);
    }
    
    void StyleInt::deserialize(StreamReader& reader)
    {
        keyword = static_cast<StyleKeyword>(reader.read<uint8_t>());
        value = reader.read<int>();
    }
    
    void StyleBool::serialize(StreamWriter& writer) const
    {
        writer.write(static_cast<uint8_t>(keyword));
        writer.write(value);
    }
    
    void StyleBool::deserialize(StreamReader& reader)
    {
        keyword = static_cast<StyleKeyword>(reader.read<uint8_t>());
        value = reader.read<bool>();
    }
} 