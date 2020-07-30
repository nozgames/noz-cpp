///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_ContentControl_h__
#define __noz_ContentControl_h__

namespace noz {

  class ContentControl : public Control {
    NOZ_OBJECT()

    NOZ_CONTROL_PART(Name=SpriteNode)
    protected: ObjectPtr<SpriteNode> sprite_node_;

    NOZ_CONTROL_PART(Name=TextNode)
    protected: ObjectPtr<TextNode> text_node_;

    NOZ_PROPERTY(Name=Text,Set=SetText)
    protected: String text_;

    NOZ_PROPERTY(Name=Sprite,Set=SetSprite)
    protected: ObjectPtr<Sprite> sprite_;

    /// Default ContentControl constructor
    public: ContentControl (void);

    public: const String& GetText(void) const {return text_;}
    public: Sprite* GetSprite (void) const {return sprite_;}

    /// Set text content
    public: void SetText(const char* text);
    public: void SetText(const String& text) {SetText(text.ToCString());}

    /// Set sprite content
    public: void SetSprite (Sprite* sprite);

    protected: virtual bool OnApplyStyle (void) override;
    protected: virtual void OnStyleChanged (void) override;
  };

} // namespace noz


#endif //__noz_ContentControl_h__

