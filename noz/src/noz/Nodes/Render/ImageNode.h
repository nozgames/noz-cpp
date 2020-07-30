///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_ImageNode_h__
#define __noz_ImageNode_h__

#include "RenderNode.h"

namespace noz {
  
  class ImageNode : public RenderNode {
    NOZ_OBJECT(EditorName="Image",EditorIcon="{1950CC7F-9EC7-4EFB-AC6F-083769F8F70F}")

    private: NOZ_PROPERTY(Name=Image,Set=SetImage) ObjectPtr<Image> image_;
    private: NOZ_PROPERTY(Name=Color,Set=SetColor) Color color_;
    private: NOZ_PROPERTY(Name=Stretch,Set=SetStretch) Stretch stretch_;
    private: NOZ_PROPERTY(Name=StretchAlignment,Set=SetStretchAlignment) Alignment stretch_alignment_;
    private: NOZ_PROPERTY(Name=Tiling,Set=SetTiling) Vector2 tiling_;

    private: RenderMesh mesh_;

    public: ImageNode (void);

    /// Set the image to render.
    public: void SetImage (Image* image);

    /// Set the color to render the image with
    public: void SetColor(Color color);

    /// Set the stretch mode of the image
    public: void SetStretch (Stretch stretch);

    /// Set the alignment used by the stretch algorithm.  When using a uniform stretch if 
    /// one of the dimensions is smaller than the target rectangle the alignment will be used
    /// to position the image within the rectangle.
    public: void SetStretchAlignment (Alignment a);

    public: void SetTiling (const Vector2& tiling);

    protected: virtual bool DrawMesh (RenderContext* rc) override;

    protected: virtual void UpdateMesh (const Rect& r) override;

    protected: virtual Vector2 MeasureMesh (const Vector2& a) override;
  };

} // namespace noz


#endif // __noz_ImageNode_h__

