#pragma once

struct SDL_GPUDevice;
struct SDL_GPUTexture;

namespace noz::renderer
{
    class Texture;
}

namespace noz
{
    /**
     * @brief Raw image data container with pixel manipulation capabilities
     * Supports both 24-bit (RGB) and 32-bit (RGBA) formats
     */
    class Image
    {
    public:
        enum class Format
        {
            RGB,    // 24-bit RGB
            RGBA    // 32-bit RGBA
        };

        // Constructors
        Image();
        Image(int width, int height, Format format = Format::RGBA);
        Image(int width, int height, const Color& fillColor);
        ~Image() = default;

        // Copy and move semantics
        Image(const Image& other) = default;
        Image& operator=(const Image& other) = default;
        Image(Image&& other) = default;
        Image& operator=(Image&& other) = default;

        // Basic properties
        int width() const { return _width; }
        int height() const { return _height; }
        Format format() const { return _format; }
        size_t pixelCount() const { return static_cast<size_t>(_width) * _height; }
        size_t dataSize() const { return pixelCount() * bytesPerPixel(); }
        int bytesPerPixel() const { return _format == Format::RGBA ? 4 : 3; }
        bool empty() const { return _width == 0 || _height == 0 || _data.empty(); }

        // Data access
        const uint8_t* data() const { return _data.data(); }
        uint8_t* data() { return _data.data(); }
        const std::vector<uint8_t>& buffer() const { return _data; }

        // Pixel access (bounds checked)
        Color32 getPixel32(int x, int y) const;
        Color24 getPixel24(int x, int y) const;
        Color getPixel(int x, int y) const;
        
        void setPixel(int x, int y, const Color32& color);
        void setPixel(int x, int y, const Color24& color);
        void setPixel(int x, int y, const Color& color);

        // Bulk operations
        void fill(const Color& color);
        void fill(const Color32& color);
        void clear() { fill(Color::Transparent); }

        // Image manipulation
        void resize(int newWidth, int newHeight);
        void convertFormat(Format newFormat);
        Image cropped(int x, int y, int width, int height) const;
        
        // Texture conversion
        static Image fromTexture(const std::shared_ptr<noz::renderer::Texture>& texture);
        static Image fromTexture(SDL_GPUDevice* device, SDL_GPUTexture* gpuTexture, int width, int height);

        // File I/O (if needed later)
        bool saveToFile(const std::string& filename) const;
        static Image loadFromFile(const std::string& filename);

        // Utility methods
        bool isValidCoordinate(int x, int y) const { return x >= 0 && x < _width && y >= 0 && y < _height; }

    private:
        int _width = 0;
        int _height = 0;
        Format _format = Format::RGBA;
        std::vector<uint8_t> _data;

        // Helper methods
        size_t getPixelIndex(int x, int y) const;
        void ensureValidSize();
    };

} // namespace noz