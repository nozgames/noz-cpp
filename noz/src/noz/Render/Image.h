///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_Graphics_Image_h__
#define __noz_Graphics_Image_h__

#include <noz/Platform/ImageHandle.h>

namespace noz {

  NOZ_ENUM() enum class ImageFormat {
    Unknown = -1,
    R8G8B8,
    R8G8B8A8,      
    A8,
    SDF,
    Count
  };

  NOZ_ENUM() enum class ImageWrapMode {
    Repeat,
    Clamp
  };

  NOZ_ENUM() enum class ImageFilterMode {
    None,
    Linear,
    Automatic
  };

  class Image : public Asset {
    NOZ_OBJECT(Managed,EditorNoCreate,EditorIcon="{1950CC7F-9EC7-4EFB-AC6F-083769F8F70F}")

    protected: ImageFormat format_;

    protected: noz_int32 width_;

    protected: noz_int32 height_;

    NOZ_PROPERTY(Name=Buffer,Type=std::vector<noz_byte>,Get=GetBuffer,Deserialize=DeserializeBuffer,Serialize=SerializeBuffer)
    private: std::vector<noz_byte>& GetBuffer(void) const {static std::vector<noz_byte> b; return b;}

    private: NOZ_PROPERTY(Name=WrapMode,Set=SetWrapMode) ImageWrapMode wrap_mode_;
    private: NOZ_PROPERTY(Name=FilterMode,Set=SetFilterMode) ImageFilterMode filter_mode_;
    
    protected: noz_byte* locked_;

    /// Platform implementation of the image.
    protected: Platform::ImageHandle* handle_;

    /// Create an uninitialized image.
    public: Image(void);

    /// Create an empty image with the given parameters.
    public: Image(noz_int32 width, noz_int32 height, ImageFormat format);

    public: ~Image(void);

    public: noz_int32 GetWidth(void) const {return width_;}

    public: noz_int32 GetHeight(void) const {return height_;}

    public: Vector2 GetSize (void) const {return Vector2((noz_float)width_,(noz_float)height_);}
    
    public: noz_uint32 GetSizeInBytes (void) const {return width_ * GetDepth() * height_;}

    public: noz_int32 GetStride(void) const {return width_ * GetDepth();}

    public: ImageFormat GetFormat(void) const {return format_;}

    public: ImageWrapMode GetWrapMode (void) const {return wrap_mode_;}

    public: ImageFilterMode GetFilterMode (void) const {return filter_mode_;}

    public: void SetWrapMode (ImageWrapMode wrap);

    public: void SetFilterMode (ImageFilterMode mode);

    public: bool HasAlpha(void) const {
      static const bool alpha[]={false,true,true,true}; return alpha[(noz_int32)format_];}

    public: bool IsLocked(void) const {return locked_ != nullptr;}

    public: noz_int32 GetDepth(void) const {
      static const noz_int32 depth[(noz_int32)ImageFormat::Count] = {3,4,1,1};
      return depth[(noz_int32)format_];
    }

    public: Platform::ImageHandle* GetHandle(void) const {return handle_;}

    public: void Save (Stream* stream);

    public: noz_byte* Lock(void);
    
    public: void Unlock(void);

    public: void SetFrom (Image* image);

    private: bool SerializeBuffer (Serializer& s);

    private: bool DeserializeBuffer (Deserializer& s);
  };

} // namespace noz


#endif // __noz_Graphics_Image_h__

