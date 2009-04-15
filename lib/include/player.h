#ifndef __PLAYER_H_DEFINED__
#define __PLAYER_H_DEFINED__

#include <sound.h>
#include <module.h>

namespace ZXTune
{
  /// Player interface
  class Player
  {
  public:
    typedef std::auto_ptr<Player> Ptr;

    virtual ~Player()
    {
    }

    /// Current player information
    struct Info
    {
      Module::Type Type;
      StringMap Properties;
    };

    /// Module playing state
    enum State
    {
      MODULE_PLAYING,
      MODULE_STOPPED
    };

    /// Retrieving player info itself
    virtual void GetInfo(Info& info) const = 0;

    /// Retrieving information about loaded module
    virtual void GetModuleInfo(Module::Information& info) const = 0;

    /// Retrieving current state of loaded module
    virtual State GetModuleState(Module::Time& timeState, Module::Tracking& trackState) const = 0;

    /// Retrieving current state of sound
    virtual State GetSoundState(Sound::Analyze::Volume& volState, Sound::Analyze::Spectrum& spectrumState) const = 0;

    /// Rendering frame
    virtual State RenderFrame(const Sound::Parameters& params, Sound::Receiver* receiver) = 0;

    /// Controlling
    virtual State Reset() = 0;
    virtual State SetPosition(const Module::Time& asTime) = 0;
    virtual State SetPosition(const Module::Tracking& asTracking) = 0;

    /// Common functions
    static Ptr Create(const String& filename, const Dump& data);
  };

  /// Common interface
  void GetSupportedPlayers(std::vector<Player::Info>& infos);
}

#endif //__PLAYER_H_DEFINED__
