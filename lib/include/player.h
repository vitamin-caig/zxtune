#ifndef __PLAYER_H_DEFINED__
#define __PLAYER_H_DEFINED__

#include <sound.h>
#include <module.h>

namespace ZXTune
{
  /// Player interface
  class ModulePlayer
  {
  public:
    typedef std::auto_ptr<ModulePlayer> Ptr;

    virtual ~ModulePlayer()
    {
    }

    /// Current player information
    struct Info
    {
      uint32_t Capabilities;
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
    virtual State GetModuleState(uint32_t& timeState, Module::Tracking& trackState) const = 0;

    /// Retrieving current state of sound
    virtual State GetSoundState(Sound::Analyze::Volume& volState, Sound::Analyze::Spectrum& spectrumState) const = 0;

    /// Rendering frame
    virtual State RenderFrame(const Sound::Parameters& params, Sound::Receiver* receiver) = 0;

    /// Controlling
    virtual State Reset() = 0;
    virtual State SetPosition(const uint32_t& frame) = 0;

    /// Virtual ctor
    static Ptr Create(const String& filename, const Dump& data);
  };

  /// Common interface
  void GetSupportedPlayers(std::vector<ModulePlayer::Info>& infos);
}

#endif //__PLAYER_H_DEFINED__
