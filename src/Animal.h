///////////////////////////////////////////////////////////////////////////////
// Troglobite
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __Troglobite_Animal_h__
#define __Troglobite_Animal_h__

namespace Troglobite {

  class Animal : public noz::Component {
    NOZ_OBJECT()

    public: Animal(void);

    public: virtual void Update (void) override;
    public: virtual void OnMouseDown (SystemEvent* e) override;
    public: virtual void OnMouseUp (SystemEvent* e) override;
  };

} // namespace noz


#endif //__Troglobite_Animal_h__


