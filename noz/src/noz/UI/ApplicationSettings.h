///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_ApplicationSettings_h__
#define __noz_ApplicationSettings_h__

namespace noz {
 
  class ApplicationSettings : public Object {    
    NOZ_OBJECT()

    private: NOZ_PROPERTY(Name=MainScene) Guid main_scene_guid_;
    private: NOZ_PROPERTY(Name=Name) Name name_;

    private: static ApplicationSettings* this_;

    protected: ApplicationSettings (void);

    public: static void Initialize (void);

    public: static void Uninitialize (void);

    public: static const Guid& GetMainSceneGuid (void) {return this_->main_scene_guid_;}

    public: static const Name& GetName (void) {return this_->name_;}
  };

} // namespace noz


#endif //__noz_ApplicationSettings_h__
