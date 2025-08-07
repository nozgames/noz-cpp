/*

	NoZ Game Engine

	Copyright(c) 2025 NoZ Games, LLC

*/

#include <SDL3_image/SDL_image.h>
#include <filesystem>
#include <algorithm>

namespace noz
{
    Image::Image()
    {
    }

    Image::Image(int width, int height, Format format)
        : _width(width), _height(height), _format(format)
    {
        ensureValidSize();
        _data.resize(dataSize());
    }

    Image::Image(int width, int height, const Color& fillColor)
        : _width(width), _height(height), _format(Format::RGBA)
    {
        ensureValidSize();
        _data.resize(dataSize());
        fill(fillColor);
    }

    void Image::ensureValidSize()
    {
        if (_width < 0) _width = 0;
        if (_height < 0) _height = 0;
    }

    size_t Image::getPixelIndex(int x, int y) const
    {
        assert(isValidCoordinate(x, y));
        return static_cast<size_t>(y * _width + x) * bytesPerPixel();
    }

    Color32 Image::getPixel32(int x, int y) const
    {
        if (!isValidCoordinate(x, y))
            return Color32::Transparent;

        size_t index = getPixelIndex(x, y);
        if (_format == Format::RGBA)
        {
            return Color32(_data[index], _data[index + 1], _data[index + 2], _data[index + 3]);
        }
        else
        {
            return Color32(_data[index], _data[index + 1], _data[index + 2], 255);
        }
    }

    Color24 Image::getPixel24(int x, int y) const
    {
        if (!isValidCoordinate(x, y))
            return Color24::Black;

        size_t index = getPixelIndex(x, y);
        return Color24(_data[index], _data[index + 1], _data[index + 2]);
    }

    Color Image::getPixel(int x, int y) const
    {
        return Color(getPixel32(x, y));
    }

    void Image::setPixel(int x, int y, const Color32& color)
    {
        if (!isValidCoordinate(x, y))
            return;

        size_t index = getPixelIndex(x, y);
        _data[index] = color.r;
        _data[index + 1] = color.g;
        _data[index + 2] = color.b;
        
        if (_format == Format::RGBA)
        {
            _data[index + 3] = color.a;
        }
    }

    void Image::setPixel(int x, int y, const Color24& color)
    {
        setPixel(x, y, Color32(color, 255));
    }

    void Image::setPixel(int x, int y, const Color& color)
    {
        setPixel(x, y, Color32(color));
    }

    void Image::fill(const Color& color)
    {
        fill(Color32(color));
    }

    void Image::fill(const Color32& color)
    {
        if (empty())
            return;

        int bpp = bytesPerPixel();
        for (size_t i = 0; i < pixelCount(); ++i)
        {
            size_t index = i * bpp;
            _data[index] = color.r;
            _data[index + 1] = color.g;
            _data[index + 2] = color.b;
            
            if (_format == Format::RGBA)
            {
                _data[index + 3] = color.a;
            }
        }
    }

    void Image::resize(int newWidth, int newHeight)
    {
        if (newWidth == _width && newHeight == _height)
            return;

        _width = newWidth;
        _height = newHeight;
        ensureValidSize();
        _data.resize(dataSize());
    }

    void Image::convertFormat(Format newFormat)
    {
        if (_format == newFormat)
            return;

        std::vector<uint8_t> newData;
        int newBpp = (newFormat == Format::RGBA) ? 4 : 3;
        newData.resize(pixelCount() * newBpp);

        if (_format == Format::RGB && newFormat == Format::RGBA)
        {
            // RGB -> RGBA: Add alpha channel
            for (size_t i = 0; i < pixelCount(); ++i)
            {
                size_t oldIndex = i * 3;
                size_t newIndex = i * 4;
                newData[newIndex] = _data[oldIndex];         // R
                newData[newIndex + 1] = _data[oldIndex + 1]; // G
                newData[newIndex + 2] = _data[oldIndex + 2]; // B
                newData[newIndex + 3] = 255;                 // A (opaque)
            }
        }
        else if (_format == Format::RGBA && newFormat == Format::RGB)
        {
            // RGBA -> RGB: Remove alpha channel
            for (size_t i = 0; i < pixelCount(); ++i)
            {
                size_t oldIndex = i * 4;
                size_t newIndex = i * 3;
                newData[newIndex] = _data[oldIndex];         // R
                newData[newIndex + 1] = _data[oldIndex + 1]; // G
                newData[newIndex + 2] = _data[oldIndex + 2]; // B
            }
        }

        _format = newFormat;
        _data = std::move(newData);
    }

    Image Image::cropped(int x, int y, int width, int height) const
    {
        // Clamp crop region to image bounds
        x = std::max(0, std::min(x, _width));
        y = std::max(0, std::min(y, _height));
        width = std::max(0, std::min(width, _width - x));
        height = std::max(0, std::min(height, _height - y));

        Image result(width, height, _format);
        
        for (int row = 0; row < height; ++row)
        {
            for (int col = 0; col < width; ++col)
            {
                result.setPixel(col, row, getPixel32(x + col, y + row));
            }
        }

        return result;
    }

    Image Image::fromTexture(const std::shared_ptr<noz::renderer::Texture>& texture)
    {
        // TODO: Implement texture readback when renderer supports it
        std::cerr << "Image::fromTexture not yet implemented - need GPU readback support" << std::endl;
        return Image();
    }

    Image Image::fromTexture(SDL_GPUDevice* device, SDL_GPUTexture* gpuTexture, int width, int height)
    {
        // TODO: Implement GPU texture readback
        std::cerr << "Image::fromTexture(GPU) not yet implemented - need SDL GPU readback support" << std::endl;
        return Image(width, height, Format::RGBA);
    }

    bool Image::saveToFile(const std::string& filename) const
    {
        if (empty())
        {
            std::cerr << "Cannot save empty image" << std::endl;
            return false;
        }

        // Create directory if it doesn't exist
        std::filesystem::path filePath(filename);
        if (auto parentPath = filePath.parent_path(); !parentPath.empty())
        {
            std::filesystem::create_directories(parentPath);
        }

        // Create SDL surface from image data
        SDL_PixelFormat pixelFormat;
        int pitch;
        
        if (_format == Format::RGBA)
        {
            pixelFormat = SDL_PIXELFORMAT_RGBA32;
            pitch = _width * 4;
        }
        else // RGB
        {
            pixelFormat = SDL_PIXELFORMAT_RGB24;
            pitch = _width * 3;
        }

        SDL_Surface* surface = SDL_CreateSurfaceFrom(
            _width, _height, pixelFormat,
            const_cast<void*>(static_cast<const void*>(_data.data())), pitch);

        if (!surface)
        {
            std::cerr << "Failed to create SDL surface: " << SDL_GetError() << std::endl;
            return false;
        }

        // Determine file format from extension and save
        std::string extension = filePath.extension().string();
        std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
        
        bool success = false;
        if (extension == ".png")
        {
            success = IMG_SavePNG(surface, filename.c_str());
        }
        else if (extension == ".jpg" || extension == ".jpeg")
        {
            success = IMG_SaveJPG(surface, filename.c_str(), 90); // 90% quality
        }
        else
        {
            std::cerr << "Unsupported file format: " << extension << std::endl;
        }

        SDL_DestroySurface(surface);

        if (!success)
        {
            std::cerr << "Failed to save image: " << SDL_GetError() << std::endl;
        }

        return success;
    }

    Image Image::loadFromFile(const std::string& filename)
    {
        // TODO: Implement image loading (can use stb_image or similar)
        std::cerr << "Image::loadFromFile not yet implemented" << std::endl;
        return Image();
    }
}
