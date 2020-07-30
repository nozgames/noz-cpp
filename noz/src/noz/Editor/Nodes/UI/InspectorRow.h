///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_Editor_InspectorRow_h__
#define __noz_Editor_InspectorRow_h__

#include <noz/Nodes/Render/TextNode.h>
#include <noz/Nodes/UI/ContentControl.h>

namespace noz {
namespace Editor {

  class InspectorRow : public ContentControl {
    NOZ_OBJECT(DefaultStyle="{B5861A1B-27B8-41E5-BCC8-F40361FFB7A1}")

    NOZ_CONTROL_PART(Name=ContentContainer)
    private: ObjectPtr<Node> content_container_;

    private: Name text_;

    public: InspectorRow (void);

    public: ~InspectorRow (void);

    protected: virtual bool OnApplyStyle (void) override;
    protected: virtual void OnStyleChanged (void) override;
  };

} // namespace Editor
} // namespace noz


#endif //__noz_Editor_InspectorRow_h__

