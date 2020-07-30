///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_ViewBox_h__
#define __noz_ViewBox_h__

namespace noz {

  class ViewBox : public UINode {
    NOZ_OBJECT()  

    private: NOZ_PROPERTY(Name=Stretch,Set=SetStretch) Stretch stretch_;

    private: ObjectPtr<Node> scale_node_;

    public: ViewBox (void);

    public: void SetStretch (Stretch stretch);

    protected: virtual void OnChildAdded (Node* n) override;
    protected: virtual void ArrangeChildren (const Rect& r) override;
    protected: virtual Vector2 MeasureChildren (const Vector2& a) override;
  };

} // namespace noz


#endif // __noz_ViewBox_h__



