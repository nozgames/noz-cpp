/*

	NoZ Game Engine

	Copyright(c) 2025 NoZ Games, LLC

*/

#pragma once

#include <noz/Asset.h>
#include <noz/ui/Style.h>

namespace noz
{
    class StreamReader;
    class StreamWriter;    
}

namespace noz::ui
{
    class StyleSheet : public noz::Asset
    {
    public:

		NOZ_DECLARE_TYPEID(StyleSheet, noz::Asset)

        using ReloadCallback = std::function<void()>;
                
        ~StyleSheet();
       
        // Save to file
        bool saveToFile(const std::string& filePath) const;
        
        // Serialization
        void serialize(StreamWriter& writer) const;
        void deserialize(StreamReader& reader);
        
        // Asset interface
        void reload() override;
        
        void setStyle(const std::string& className, const Style& style);
		void mergeStyle(const std::string& className, const Style& style);
        const Style* style(const std::string& className) const;
        bool hasStyle(const std::string& className) const;
        
        // Inheritance management
        void addInheritedStyleSheet(std::shared_ptr<StyleSheet> stylesheet);
        void clearInheritedStyleSheets();
        
        // Style resolution with inheritance fallback
        Style resolveStyle(const std::string& className) const;
                
        // Clear all styles
        void clear();
        
    private:

        friend class AssetDatabase;

        static std::shared_ptr<StyleSheet> load(const std::string& name);

        StyleSheet();

        void loadInternal();

        std::unordered_map<std::string, Style> _styles;
    };
}