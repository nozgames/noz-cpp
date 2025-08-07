/*

	NoZ Game Engine

	Copyright(c) 2025 NoZ Games, LLC

*/

#pragma once

#include <noz/IResource.h>
#include <noz/ui/Style.h>

namespace noz
{
    class StreamReader;
    class StreamWriter;    
}

namespace noz::ui
{
    class StyleSheet : public noz::IResource
    {
    public:

		NOZ_DECLARE_TYPEID(StyleSheet, noz::IResource)

        using ReloadCallback = std::function<void()>;
        
        StyleSheet(const std::string& path = "");
        ~StyleSheet();

        // Load from file
        bool loadFromFile(const std::string& filePath);
        
        // Static load method for resource system
        static std::shared_ptr<StyleSheet> load(const std::string& name);
        
        // Save to file
        bool saveToFile(const std::string& filePath) const;
        
        // Serialization
        void serialize(StreamWriter& writer) const;
        void deserialize(StreamReader& reader);
        
        // IResource interface
        void reload() override;
        
        // Style management
        void addStyle(const std::string& className, const Style& style);
        const Style* getStyle(const std::string& className) const;
        bool hasStyle(const std::string& className) const;
        
        // Inheritance management
        void addInheritedStyleSheet(std::shared_ptr<StyleSheet> stylesheet);
        void clearInheritedStyleSheets();
        const std::vector<std::shared_ptr<StyleSheet>>& inheritedStyleSheets() const { return _inheritedStyleSheets; }
        
        // Style resolution with inheritance fallback
        Style resolveStyle(const std::string& className) const;
                
        // Clear all styles
        void clear();
        
    private:
        std::unordered_map<std::string, Style> _styles;
        std::vector<std::shared_ptr<StyleSheet>> _inheritedStyleSheets;
        
        // Helper to find style in inheritance chain
        const Style* findStyleInChain(const std::string& className) const;
    };
}