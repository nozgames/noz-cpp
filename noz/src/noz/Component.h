///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_Component_h__
#define __noz_Component_h__

namespace noz {

  class Component;
  class Node;
  class Scene;

  class Component : public Object {
    NOZ_OBJECT(Abstract)

    friend class Node;

    private: Node* node_;

    private: bool enabled_;

    public: Component (void);

    public: ~Component (void);

    public: Node* GetNode(void) const {return node_;}

    public: void SetEnabled (bool e=true);

    public: bool IsEnabled (void) const {return enabled_;}

    protected: void InvalidateTransform (void);

    protected: virtual void OnAwake (void) {};

    protected: virtual void OnKeyDown (SystemEvent* e) {}

    protected: virtual void OnKeyUp (SystemEvent* e) {}

    protected: virtual void OnMouseDown (SystemEvent* e) {}

    protected: virtual void OnMouseEnter (void) { }

    protected: virtual void OnMouseLeave (void) { }
    
    protected: virtual void OnMouseOver (SystemEvent* e) {}

    protected: virtual void OnMouseUp (SystemEvent* e) {}

    protected: virtual void OnMouseWheel (SystemEvent* e) {}

    protected: virtual void Update (void) { }

    protected: virtual void OnDetach (Node* node) { }

    protected: virtual void OnEnabled (void) { }

    protected: virtual void OnDisabled (void) { }
  };

} // namespace noz

#endif //__noz_Component_h__


