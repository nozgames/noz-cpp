///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_Assets_Asset_h__
#define __noz_Assets_Asset_h__

namespace noz {

  class AssetManager;

  class Asset : public Object {
    NOZ_OBJECT(Abstract)

    friend class AssetManager;

    /// True if the asset is managed by the asset manager.
    private: bool managed_;

    /// Unique identifier within session.
    private: noz_uint64 uid_;

    /// Name of the asset
    private: Name name_;

    /// Guid of the asset
    private: Guid guid_;

    /// Default constructor
    protected: Asset(void);

    /// Return the name of the asset
    public: const Name& GetName(void) const {return name_;}

    /// Return the session unique identifier of the asset
    public: noz_uint64 GetUID(void) const {return uid_;}

    /// Return the globally unique identifier of the asset
    public: const Guid& GetGuid (void) const {return guid_;}

    /// Return true if the asset is being managed by an asset manager
    public: bool IsManaged(void) const {return managed_; }

    /// Called by the asset manager after the asset has been completely loaded into memory
    public: virtual void OnLoaded (void) {}
  };

} // namespace noz

#endif // __noz_Assets_Asset_h__
