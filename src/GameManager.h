///////////////////////////////////////////////////////////////////////////////
// Troglobite
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#ifndef __FarmerZ_MainMenu_h__
#define __FarmerZ_MainMenu_h__

namespace FarmerZ {

  class GameManager : public noz::Behavior {
    NOZ_OBJECT()

    private: NOZ_PROPERTY(Name=PlayButton) noz::ObjectPtr<noz::Button> play_button_;
    private: NOZ_PROPERTY(Name=PlayButtonSound) noz::ObjectPtr<noz::AudioClip> play_button_sound_;
    private: NOZ_PROPERTY(Name=Board) noz::ObjectPtr<noz::Node> board_;
    private: NOZ_PROPERTY(Name=Animals) std::vector<noz::ObjectPtr<noz::Prefab>> animals_;

    private: static GameManager* this_;

    public: GameManager (void);

    public: ~GameManager (void);

    public: static void TransitionIn (void);
    public: static void TransitionOut (void);

    public: virtual void OnStart (void) override;
    public: virtual void OnUpdate (void) override;

    private: void OnPlayButton (UINode*);

    private: NOZ_METHOD() void StateResetGame (void);
    private: NOZ_METHOD() void StateStartGame (void);

    private: void ResetBoard (void);
  };

} // namespace FarmerZ


#endif //__FarmerZ_MainMenu_h__


