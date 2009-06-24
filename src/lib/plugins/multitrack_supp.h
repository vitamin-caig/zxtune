#ifndef __MULTITRACK_SUPP_H_DEFINED__
#define __MULTITRACK_SUPP_H_DEFINED__

#include <player.h>

#include "../io/container.h"

namespace ZXTune
{
  class MultitrackBase : public ModulePlayer
  {
  public:
    explicit MultitrackBase(const String& selfName);

    bool GetPlayerInfo(Info& info) const;

    /// Retrieving information about loaded module
    virtual void GetModuleInfo(Module::Information& info) const;

    /// Retrieving current state of loaded module
    virtual State GetModuleState(std::size_t& timeState, Module::Tracking& trackState) const;

    /// Retrieving current state of sound
    virtual State GetSoundState(Sound::Analyze::ChannelsState& state) const;

    /// Rendering frame
    virtual State RenderFrame(const Sound::Parameters& params, Sound::Receiver& receiver);

    /// Controlling
    virtual State Reset();
    virtual State SetPosition(std::size_t frame);
  protected:
    class SubmodulesIterator
    {
    public:
      virtual ~SubmodulesIterator()
      {
      }

      virtual void Reset() = 0;
      virtual void Reset(const String& filename) = 0;
      virtual bool Get(String& filename, IO::DataContainer::Ptr& container) const = 0;
      virtual void Next() = 0;
    };
    //called at the end of ctor
    void Process(SubmodulesIterator& iterator, uint32_t capFilter);
  private:
    const String Filename;
    ModulePlayer::Ptr Delegate;
    Module::Information Information;
  };
}

#endif //__MULTITRACK_SUPP_H_DEFINED__
