/*

	NoZ Game Engine

	Copyright(c) 2025 NoZ Games, LLC

*/

#include <noz/MetaFile.h>
#include <sstream>
#include <algorithm>
#include <set>

namespace noz
{
    MetaFile MetaFile::parse(const std::string& filePath)
    {
        MetaFile metaFile;
        std::ifstream file(filePath);
            
        if (!file.is_open())
            return metaFile;

        std::string line;
        while (std::getline(file, line))
            metaFile.parseLine(line);

        file.close();
        return metaFile;
    }

    bool MetaFile::getBool(const std::string& group, const std::string& key, bool defaultValue) const
    {
        std::string fullKey = makeKey(group, key);
        auto it = _data.find(fullKey);
        if (it == _data.end())
            return defaultValue;

        std::string value = it->second;
        std::transform(value.begin(), value.end(), value.begin(), ::tolower);
            
        return value == "true" || value == "1" || value == "yes" || value == "on";
    }

    int MetaFile::getInt(const std::string& group, const std::string& key, int defaultValue) const
    {
        std::string fullKey = makeKey(group, key);
        auto it = _data.find(fullKey);
        if (it == _data.end())
            return defaultValue;

        try
        {
            return std::stoi(it->second);
        }
        catch (const std::exception&)
        {
            return defaultValue;
        }
    }

    float MetaFile::getFloat(const std::string& group, const std::string& key, float defaultValue) const
    {
        std::string fullKey = makeKey(group, key);
        auto it = _data.find(fullKey);
        if (it == _data.end())
            return defaultValue;

        try
        {
            return std::stof(it->second);
        }
        catch (const std::exception&)
        {
            return defaultValue;
        }
    }

    std::string MetaFile::getString(const std::string& group, const std::string& key, const std::string& defaultValue) const
    {
        std::string fullKey = makeKey(group, key);
        auto it = _data.find(fullKey);
        if (it == _data.end())
            return defaultValue;

        return it->second;
    }

    bool MetaFile::hasKey(const std::string& group, const std::string& key) const
    {
        std::string fullKey = makeKey(group, key);
        return _data.find(fullKey) != _data.end();
    }

    bool MetaFile::hasKey(const std::string& key) const
    {
        return hasKey("", key);
    }

    void MetaFile::parseLine(const std::string& line)
    {
        std::string trimmedLine = trim(line);
            
        // Skip empty lines and comments
        if (trimmedLine.empty() || isComment(trimmedLine))
            return;

        // Check if this is a section header [group]
        if (isSection(trimmedLine))
        {
            _currentGroup = extractSection(trimmedLine);
            return;
        }

        // Parse key=value format (support both = and :)
        size_t equalPos = trimmedLine.find('=');
        if (equalPos == std::string::npos)
        {
            equalPos = trimmedLine.find(':');
            if (equalPos == std::string::npos)
                return;
        }

        std::string key = trim(trimmedLine.substr(0, equalPos));
        std::string value = trim(trimmedLine.substr(equalPos + 1));

        // Remove quotes if present
        if (value.length() >= 2 && value.front() == '"' && value.back() == '"')
        {
            value = value.substr(1, value.length() - 2);
        }

        // Store with full key (group.key or just key if no group)
        std::string fullKey = makeKey(_currentGroup, key);
        _data[fullKey] = value;
    }

    std::string MetaFile::trim(const std::string& str) const
    {
        size_t start = str.find_first_not_of(" \t\r\n");
        if (start == std::string::npos)
            return "";

        size_t end = str.find_last_not_of(" \t\r\n");
        return str.substr(start, end - start + 1);
    }

    bool MetaFile::isComment(const std::string& line) const
    {
        std::string trimmed = trim(line);
        return trimmed.empty() || trimmed[0] == '#' || trimmed[0] == ';';
    }
    
    std::vector<std::string> MetaFile::groups() const
    {
        std::set<std::string> uniqueGroups;
        
        for (const auto& [fullKey, value] : _data)
        {
            size_t dotPos = fullKey.find('.');
            if (dotPos != std::string::npos)
            {
                std::string group = fullKey.substr(0, dotPos);
                uniqueGroups.insert(group);
            }
            else if (!fullKey.empty())
            {
                uniqueGroups.insert(""); // Empty group for legacy keys
            }
        }
        
        return std::vector<std::string>(uniqueGroups.begin(), uniqueGroups.end());
    }
    
    std::vector<std::string> MetaFile::keys(const std::string& group) const
    {
        std::vector<std::string> result;
        std::string prefix = group.empty() ? "" : (group + ".");
        
        for (const auto& [fullKey, value] : _data)
        {
            if (group.empty())
            {
                // For empty group, only include keys without dots
                if (fullKey.find('.') == std::string::npos)
                {
                    result.push_back(fullKey);
                }
            }
            else
            {
                // For specific group, check if key starts with group prefix
                if (fullKey.substr(0, prefix.length()) == prefix)
                {
                    std::string key = fullKey.substr(prefix.length());
                    result.push_back(key);
                }
            }
        }
        
        return result;
    }
    
    bool MetaFile::isSection(const std::string& line) const
    {
        std::string trimmed = trim(line);
        return !trimmed.empty() && trimmed.front() == '[' && trimmed.back() == ']';
    }
    
    std::string MetaFile::extractSection(const std::string& line) const
    {
        std::string trimmed = trim(line);
        if (trimmed.length() < 2 || trimmed.front() != '[' || trimmed.back() != ']')
            return "";
            
        return trim(trimmed.substr(1, trimmed.length() - 2));
    }
    
    std::string MetaFile::makeKey(const std::string& group, const std::string& key) const
    {
        if (group.empty())
            return key;
        return group + "." + key;
    }

    Color MetaFile::getColor(const std::string& group, const std::string& key, const Color& defaultValue) const
    {
        std::string fullKey = makeKey(group, key);
        auto it = _data.find(fullKey);
        if (it == _data.end())
            return defaultValue;
            
        std::string propertyValue = it->second;
        
        // Parse hex color values like "#FFFFFF" or "#FFF"
        if (propertyValue.length() > 0 && propertyValue[0] == '#')
        {
            std::string hex = propertyValue.substr(1);
            
            // Convert to uppercase for consistency
            std::transform(hex.begin(), hex.end(), hex.begin(), ::toupper);
            
            // Handle 3-character hex (#FFF -> #FFFFFF)
            if (hex.length() == 3)
            {
                hex = std::string(1, hex[0]) + hex[0] + hex[1] + hex[1] + hex[2] + hex[2];
            }
            
            // Parse 6-character hex
            if (hex.length() == 6)
            {
                try
                {
                    int r = std::stoi(hex.substr(0, 2), nullptr, 16);
                    int g = std::stoi(hex.substr(2, 2), nullptr, 16);
                    int b = std::stoi(hex.substr(4, 2), nullptr, 16);
                    
                    return Color(
                        r / 255.0f,
                        g / 255.0f,
                        b / 255.0f,
                        1.0f
                    );
                }
                catch (const std::exception&)
                {
                    // Fall through to default value
                }
            }
            // Handle 8-character hex with alpha (#FFFFFFAA)
            else if (hex.length() == 8)
            {
                try
                {
                    int r = std::stoi(hex.substr(0, 2), nullptr, 16);
                    int g = std::stoi(hex.substr(2, 2), nullptr, 16);
                    int b = std::stoi(hex.substr(4, 2), nullptr, 16);
                    int a = std::stoi(hex.substr(6, 2), nullptr, 16);
                    
                    return Color(
                        r / 255.0f,
                        g / 255.0f,
                        b / 255.0f,
                        a / 255.0f
                    );
                }
                catch (const std::exception&)
                {
                    // Fall through to default value
                }
            }
        }
        // Parse color values like "rgba(255, 255, 255, 1)"
        else if (propertyValue.substr(0, 5) == "rgba(")
        {
            // Parse rgba(r, g, b, a)
            std::string values = propertyValue.substr(5, propertyValue.length() - 6);
            std::istringstream stream(values);
            std::string value;
            std::vector<float> components;
            
            while (std::getline(stream, value, ','))
            {
                value.erase(0, value.find_first_not_of(" \t"));
                value.erase(value.find_last_not_of(" \t") + 1);
                try
                {
                    components.push_back(std::stof(value));
                }
                catch (const std::exception&)
                {
                    components.push_back(0.0f);
                }
            }
            
            if (components.size() >= 4)
            {
                return Color(
                    components[0] / 255.0f,
                    components[1] / 255.0f,
                    components[2] / 255.0f,
                    components[3]
                );
            }
        }
        else if (propertyValue.substr(0, 4) == "rgb(")
        {
            // Parse rgb(r, g, b) - assume alpha = 1.0
            std::string values = propertyValue.substr(4, propertyValue.length() - 5);
            std::istringstream stream(values);
            std::string value;
            std::vector<float> components;
            
            while (std::getline(stream, value, ','))
            {
                value.erase(0, value.find_first_not_of(" \t"));
                value.erase(value.find_last_not_of(" \t") + 1);
                try
                {
                    components.push_back(std::stof(value));
                }
                catch (const std::exception&)
                {
                    components.push_back(0.0f);
                }
            }
            
            if (components.size() >= 3)
            {
                return Color(
                    components[0] / 255.0f,
                    components[1] / 255.0f,
                    components[2] / 255.0f,
                    1.0f
                );
            }
        }
        
        // Check for CSS color names
        std::string lowerValue = propertyValue;
        std::transform(lowerValue.begin(), lowerValue.end(), lowerValue.begin(), ::tolower);
        
        // CSS color name map
        static const std::unordered_map<std::string, Color> cssColors = {
            // Basic colors
            {"black", Color(0.0f, 0.0f, 0.0f, 1.0f)},
            {"white", Color(1.0f, 1.0f, 1.0f, 1.0f)},
            {"red", Color(1.0f, 0.0f, 0.0f, 1.0f)},
            {"green", Color(0.0f, 0.5f, 0.0f, 1.0f)},  // CSS green is darker
            {"blue", Color(0.0f, 0.0f, 1.0f, 1.0f)},
            {"yellow", Color(1.0f, 1.0f, 0.0f, 1.0f)},
            {"cyan", Color(0.0f, 1.0f, 1.0f, 1.0f)},
            {"magenta", Color(1.0f, 0.0f, 1.0f, 1.0f)},
            
            // Extended colors
            {"gray", Color(0.5f, 0.5f, 0.5f, 1.0f)},
            {"grey", Color(0.5f, 0.5f, 0.5f, 1.0f)},
            {"silver", Color(0.75f, 0.75f, 0.75f, 1.0f)},
            {"maroon", Color(0.5f, 0.0f, 0.0f, 1.0f)},
            {"olive", Color(0.5f, 0.5f, 0.0f, 1.0f)},
            {"lime", Color(0.0f, 1.0f, 0.0f, 1.0f)},
            {"aqua", Color(0.0f, 1.0f, 1.0f, 1.0f)},
            {"teal", Color(0.0f, 0.5f, 0.5f, 1.0f)},
            {"navy", Color(0.0f, 0.0f, 0.5f, 1.0f)},
            {"fuchsia", Color(1.0f, 0.0f, 1.0f, 1.0f)},
            {"purple", Color(0.5f, 0.0f, 0.5f, 1.0f)},
            {"orange", Color(1.0f, 0.647f, 0.0f, 1.0f)},
            
            // More common CSS colors
            {"brown", Color(0.647f, 0.165f, 0.165f, 1.0f)},
            {"pink", Color(1.0f, 0.753f, 0.796f, 1.0f)},
            {"gold", Color(1.0f, 0.843f, 0.0f, 1.0f)},
            {"violet", Color(0.933f, 0.51f, 0.933f, 1.0f)},
            {"indigo", Color(0.294f, 0.0f, 0.51f, 1.0f)},
            {"turquoise", Color(0.251f, 0.878f, 0.816f, 1.0f)},
            {"coral", Color(1.0f, 0.498f, 0.314f, 1.0f)},
            {"salmon", Color(0.98f, 0.502f, 0.447f, 1.0f)},
            {"khaki", Color(0.941f, 0.902f, 0.549f, 1.0f)},
            {"crimson", Color(0.863f, 0.078f, 0.235f, 1.0f)},
            {"tomato", Color(1.0f, 0.388f, 0.278f, 1.0f)},
            {"wheat", Color(0.961f, 0.871f, 0.702f, 1.0f)},
            {"tan", Color(0.824f, 0.706f, 0.549f, 1.0f)},
            {"chocolate", Color(0.824f, 0.412f, 0.118f, 1.0f)},
            {"azure", Color(0.941f, 1.0f, 1.0f, 1.0f)},
            {"beige", Color(0.961f, 0.961f, 0.863f, 1.0f)},
            {"ivory", Color(1.0f, 1.0f, 0.941f, 1.0f)},
            
            // Special keyword
            {"transparent", Color(0.0f, 0.0f, 0.0f, 0.0f)}
        };
        
        auto colorIt = cssColors.find(lowerValue);
        if (colorIt != cssColors.end())
        {
            return colorIt->second;
        }
        
        return defaultValue;
    }

	vec3 MetaFile::getVec3(const std::string& group, const std::string& key, const vec3& defaultValue) const
	{
		std::string fullKey = makeKey(group, key);
		auto it = _data.find(fullKey);
		if (it == _data.end())
			return defaultValue;
		std::string value = it->second;
		std::istringstream stream(value);
		vec3 result(0.0f, 0.0f, 0.0f);
		char delimiter;
		if (stream >> result.x >> delimiter >> result.y >> delimiter >> result.z)
		{
			return result;
		}
		return defaultValue;
	}
}
