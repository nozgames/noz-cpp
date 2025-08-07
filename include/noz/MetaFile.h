/*

	NoZ Game Engine

	Copyright(c) 2025 NoZ Games, LLC

*/

#pragma once

#include <noz/Color.h>

namespace noz
{
    class MetaFile
    {
    public:
        MetaFile() = default;
        ~MetaFile() = default;

        // Parse a meta file from a file path
        static MetaFile parse(const std::string& filePath);

        // Get values with type conversion (group-aware)
        bool getBool(const std::string& group, const std::string& key, bool defaultValue = false) const;
        int getInt(const std::string& group, const std::string& key, int defaultValue = 0) const;
        float getFloat(const std::string& group, const std::string& key, float defaultValue = 0.0f) const;
        std::string getString(const std::string& group, const std::string& key, const std::string& defaultValue = "") const;
		vec3 getVec3(const std::string& group, const std::string& key, const vec3& defaultValue = vec3(0.0f)) const;
        
        Color getColor(const std::string& group, const std::string& key, const Color& defaultValue = Color::Transparent) const;

        // Check if key exists
        bool hasKey(const std::string& group, const std::string& key) const;
        bool hasKey(const std::string& key) const; // Legacy API

        // Get all data
        const std::unordered_map<std::string, std::string>& data() const { return _data; }
        
        // Get all groups
        std::vector<std::string> groups() const;
        
        // Get all keys in a group
        std::vector<std::string> keys(const std::string& group) const;

    private:
        // Data stored as "group.key" -> "value" for easy lookup
        std::unordered_map<std::string, std::string> _data;
        std::string _currentGroup;

        // Helper methods
        void parseLine(const std::string& line);
        std::string trim(const std::string& str) const;
        bool isComment(const std::string& line) const;
        bool isSection(const std::string& line) const;
        std::string extractSection(const std::string& line) const;
        std::string makeKey(const std::string& group, const std::string& key) const;
    };
}
