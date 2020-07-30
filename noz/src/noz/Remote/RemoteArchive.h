///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_RemoteArchive_h__
#define __noz_RemoteArchive_h__

namespace noz {

  class RemoteClientView;

  class RemoteArchive : public AssetArchive {
    NOZ_OBJECT()

    private: ObjectPtr<RemoteClientView> view_;

    public: RemoteArchive (RemoteClientView* view);

    public: virtual Stream* OpenFile (const Guid& path) override;
  };

} // namespace noz


#endif // __noz_RemoteArchive_h__

