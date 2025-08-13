/*

	NoZ Game Engine

	Copyright(c) 2025 NoZ Games, LLC

*/

#include <noz/ui/StyleSheet.h>
#include <noz/ui/Style.h>

namespace noz::ui
{
	NOZ_DEFINE_TYPEID(StyleSheet)

    StyleSheet::StyleSheet()
    {
    }
    
    StyleSheet::~StyleSheet() = default;
    
    std::shared_ptr<StyleSheet> StyleSheet::load(const std::string& name)
    {
        auto styleSheet = Object::create<StyleSheet>(name);
        styleSheet->loadInternal();
        return styleSheet;
    }
    
    void StyleSheet::loadInternal()
    {
        StreamReader reader;
        if (!reader.loadFromFile(AssetDatabase::getFullPath(name(), "styles")))
			throw std::runtime_error("Failed to load stylesheet: " + name());
        
        // Verify file signature
        if (!reader.readFileSignature("STYL"))
			throw std::runtime_error("Invalid stylesheet file signature: " + name());
        
        deserialize(reader);
    }
    
    bool StyleSheet::saveToFile(const std::string& filePath) const
    {
        StreamWriter writer;
        writer.writeFileSignature("STYL");
        serialize(writer);
        
        if (!writer.writeToFile(filePath))
        {
            std::cerr << "Failed to save stylesheet to: " << filePath << std::endl;
            return false;
        }
        
        return true;
    }
    
    void StyleSheet::reload()
    {
		loadInternal();
    }
    
    void StyleSheet::serialize(StreamWriter& writer) const
    {
        // Write number of styles
        writer.write(static_cast<uint32_t>(_styles.size()));
        
        // Write each style with its class name
        for (const auto& [className, style] : _styles)
        {
            writer.writeString(className);
            style.serialize(writer);
        }
    }
    
    void StyleSheet::deserialize(StreamReader& reader)
    {
        clear();
        
        // Read number of styles
        uint32_t styleCount = reader.read<uint32_t>();
        
        // Read each style
        for (uint32_t i = 0; i < styleCount; ++i)
        {
            auto className = reader.readString();
            
			Style style;
            style.deserialize(reader);
            _styles[className] = style;
        }
    }
    
    void StyleSheet::setStyle(const std::string& className, const Style& style)
    {
        _styles[className] = style;
    }

    void StyleSheet::mergeStyle(const std::string& className, const Style& style)
    {
        auto& it = _styles.find(className);
        if (it != _styles.end())
			it->second.apply(style);
        else
            _styles[className] = style;
	}
    
    const Style* StyleSheet::style(const std::string& className) const
    {
        auto it = _styles.find(className);
        if (it != _styles.end())
            return &it->second;

        return nullptr;
    }
    
    bool StyleSheet::hasStyle(const std::string& className) const
    {
        return _styles.find(className) != _styles.end();
    }
       
    Style StyleSheet::resolveStyle(const std::string& className) const
    {
        auto resolvedStyle = Style::defaultStyle();
        resolvedStyle.apply(style(className));
        return resolvedStyle;
    }
        
    void StyleSheet::clear()
    {
        _styles.clear();
    }
}