///////////////////////////////////////////////////////////////////////////////
// NoZ Engine
// Copyright (C) 2015-2016 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __noz_Editor_AudioClipFile_h__
#define __noz_Editor_AudioClipFile_h__

namespace noz {
namespace Editor {

  class AudioClipDef : public Object {
    NOZ_OBJECT()

    public: AudioClipDef (void);

    public: ~AudioClipDef (void);
  };

  class AudioClipFile : public AssetFile {
    NOZ_OBJECT(Abstract)

    public: virtual Asset* Import (void) override;

    private: AudioClip* Import (Stream* stream, AudioClipDef& def);

    public: virtual DateTime GetModifiedDate (void) const override;
  };

  class WAVAudioClipFile : public AudioClipFile {
    NOZ_OBJECT(EditorFileExt=wav, EditorAssetType=noz::AudioClip)
  };

} // namespace Editor
} // namespace noz

#endif // __noz_Editor_AudioClipFile_h__

