///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_Sprite_h__
#define __noz_Sprite_h__

namespace noz {

  class Sprite : public Asset {
    NOZ_OBJECT(Managed,EditorIcon="{7A813716-C722-44F9-B2F9-6B2A01BF8E77}")

    NOZ_PROPERTY(Name=Rectangle,Set=SetRectangle)
    private: Rect rect_;

    NOZ_PROPERTY(Name=Image,Set=SetImage)
    private: ObjectPtr<Image> image_;

    private: Vector2 size_;
    private: Vector2 s_;
    private: Vector2 t_;

    public: Sprite(void);

    public: Sprite(Image* image, const Rect& rect);

    public: Image* GetImage (void) { return image_; }

    public: const Vector2& GetS(void) const {return s_;}
    
    public: const Vector2& GetT(void) const {return t_;}

    public: const Vector2& GetSize(void) const {return size_;}

    public: const Rect& GetRectangle (void) const {return rect_;}

    public: void SetImage (Image* image);

    public: void SetRectangle(const Rect& rect);

    /// Called internally to update the S and T values from the sprite texture and rectangle
    private: void UpdateST(void);
  };

} // namespace noz


#endif //__noz_Sprite_h__

