///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_Platform_OpenGLTexture_h__
#define __noz_Platform_OpenGLTexture_h__

namespace noz {
namespace Platform {

  class OpenGLRenderContext;

  class OpenGLTexture : public ImageHandle {
    friend class OpenGLRenderContext;

    /// GL identifier of image texture.
    protected: GLuint id_;

    /// True if the texture image is dirty and needs to be re-uploaded
    protected: bool dirty_;

    protected: bool state_dirty_;

    protected: Image* image_;

    protected: ImageFilterMode current_filter_;

    protected: std::vector<noz_byte> buffer_;

    // Default constructor
    public: OpenGLTexture(Image* image);

    // Default destructor
    public: ~OpenGLTexture(void);

    /// Bind the image as the current texture 
    public: void Bind(noz_int32 texture=0);

    public: void UnBind (void);
    
    public: GLuint GetId (void) const {return id_;}

    public: virtual noz_byte* Lock(void) override;
    public: virtual void Unlock(void) override;

    public: virtual void InvalidateState (void) override;

    public: void SetFilterMode (ImageFilterMode m);
  };
    
} // namespace Platform
} // namespace noz


#endif // __noz_Platform_OpenGLTexture_h__
