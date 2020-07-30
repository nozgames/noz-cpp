///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_SpriteNode_h__
#define __noz_SpriteNode_h__

#include "RenderNode.h"

namespace noz {

  class SpriteColors : public Component {
    NOZ_OBJECT()

    NOZ_PROPERTY(Name=TopLeft,Type=Color,Set=SetTopLeft,Get=GetTopLeft)
    NOZ_PROPERTY(Name=TopRight,Type=Color,Set=SetTopRight,Get=GetTopRight)
    NOZ_PROPERTY(Name=BottomLeft,Type=Color,Set=SetBottomLeft,Get=GetBottomLeft)
    NOZ_PROPERTY(Name=BottomRight,Type=Color,Set=SetBottomRight,Get=GetBottomRight)

    public: Color colors_[4];

    public: Color GetTopLeft (void) const {return colors_[0];}
    public: Color GetTopRight (void) const {return colors_[1];}
    public: Color GetBottomLeft (void) const {return colors_[3];}
    public: Color GetBottomRight (void) const {return colors_[2];}

    public: void SetTopLeft (Color c);
    public: void SetTopRight (Color c);
    public: void SetBottomLeft (Color c);
    public: void SetBottomRight (Color c);

    protected: virtual void OnDetach (Node* node) override;
  };

  class SpriteNode : public RenderNode {
    NOZ_OBJECT(EditorName="Sprite",EditorIcon="{7A813716-C722-44F9-B2F9-6B2A01BF8E77}")

    private: NOZ_PROPERTY(Name=Sprite,Set=SetSprite) ObjectPtr<Sprite> sprite_;
    private: NOZ_PROPERTY(Name=Color,Set=SetColor) Color color_;
    private: NOZ_PROPERTY(Name=Stretch,Set=SetStretch) Stretch stretch_;

    private: RenderMesh mesh_;

    public: SpriteNode(void);

    /// Set the sprite to render.
    public: void SetSprite (Sprite* sprite);

    /// Set the color to render the sprite with
    public: void SetColor(Color color);

    public: void SetStretch (Stretch stretch);

    public: Sprite* GetSprite (void) const {return sprite_;}

    protected: virtual void UpdateMesh (const Rect& r) override;

    protected: virtual Vector2 MeasureMesh (const Vector2& a) override;

    protected: virtual bool DrawMesh (RenderContext* rc) override;
  };

} // namespace noz


#endif // __noz_SpriteNode_h__

