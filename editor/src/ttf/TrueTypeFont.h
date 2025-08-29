//
//  NoZ Game Engine - Copyright(c) 2025 NoZ Games, LLC
//

#pragma once

struct Stream;

namespace noz::ttf
{
    class TrueTypeFont
    {
    public:

        enum class CurveType : uint8_t
        {
            None,
            Cubic,
            Conic
        };

        struct Point
        {
            glm::highp_dvec2 xy;
            CurveType curve;
        };

        struct Contour
        {
            int start;
            int length;
        };

        struct Kerning
        {
            uint32_t left;
            uint32_t right;
            float value;
        };

        struct Glyph
        {
            uint16_t id;
            char ascii;
            std::vector<Point> points;
            std::vector<Contour> contours;
            double advance;
            glm::highp_dvec2 size;
            glm::highp_dvec2 bearing;
        };

        double ascent() const { return _ascent; }
        double descent() const { return _descent; }
        double height() const { return _height; }

        const std::vector<Kerning>& kerning() const { return _kerning; }

        const Glyph* glyph(char c) const { return _glyphs[c]; }

        static TrueTypeFont* load(const std::string& path, int requestedSize, const std::string& filter);
        static TrueTypeFont* load(Stream* stream, int requestedSize, const std::string& filter);

    private:

        friend class TrueTypeFontReader;

        std::vector<Glyph*> _glyphs;
        std::vector<Kerning> _kerning;
        double _ascent;
        double _descent;
        double _height;
    };
}