#include "plugin_enumerator.h"

#include "../io/container.h"

#include <tools.h>

#include <player_attrs.h>

#include <boost/static_assert.hpp>

#include <cassert>
#include <valarray>

namespace
{
  using namespace ZXTune;

  const String TEXT_TRD_INFO("TRD modules support");
  const String TEXT_TRD_VERSION("0.1");

  const std::size_t TRD_MODULE_SIZE = 655360;

#ifdef USE_PRAGMA_PACK
#pragma pack(push,1)
#endif

  enum
  {
    NOENTRY = 0,
    DELETED = 1
  };
  PACK_PRE struct CatEntry
  {
    uint8_t Filename[8];
    uint8_t Filetype;
    uint16_t Start;
    uint16_t Length;
    uint8_t SizeInSectors;
    uint8_t Sector;
    uint8_t Track;
  } PACK_POST;

  enum
  {
    TRD_ID = 0x10,

    DS_DD = 0x16,
    DS_SD = 0x17,
    SS_DD = 0x18,
    SS_SD = 0x19
  };
  PACK_PRE struct ServiceSector
  {
    uint8_t Zero;
    uint8_t Reserved1[224];
    uint8_t FreeSpaceSect;
    uint8_t FreeSpaceTrack;
    uint8_t Type;
    uint8_t Files;
    uint16_t FreeSectors;
    uint8_t ID;//0x10
    uint8_t Reserved2[12];
    uint8_t DeletedFiles;
    uint8_t Title[8];
    uint8_t Reserved3[3];
  } PACK_POST;

#ifdef USE_PRAGMA_PACK
#pragma pack(pop)
#endif

  BOOST_STATIC_ASSERT(sizeof(CatEntry) == 16);
  BOOST_STATIC_ASSERT(sizeof(ServiceSector) == 256);

  //////////////////////////////////////////////////////////////////////////
  void Describing(ModulePlayer::Info& info);

  class PlayerImpl : public ModulePlayer
  {
  public:
    PlayerImpl(const String& filename, const IO::DataContainer& data)
    {
      Information.Capabilities = CAP_MULTITRACK;
      Information.Loop = 0;
      Information.Statistic = Module::Tracking();
    }

    /// Retrieving player info itself
    virtual void GetInfo(Info& info) const
    {
      if (Delegate.get())
      {
        Delegate->GetInfo(info);
      }
      else
      {
        Describing(info);
      }
    }

    /// Retrieving information about loaded module
    virtual void GetModuleInfo(Module::Information& info) const
    {
      if (Delegate.get())
      {
        Delegate->GetModuleInfo(info);
      }
      else
      {
        info = Information;
      }
    }

    /// Retrieving current state of loaded module
    virtual State GetModuleState(std::size_t& timeState, Module::Tracking& trackState) const
    {
      return Delegate.get() ? Delegate->GetModuleState(timeState, trackState) : MODULE_STOPPED;
    }

    /// Retrieving current state of sound
    virtual State GetSoundState(Sound::Analyze::Volume& volState, Sound::Analyze::Spectrum& spectrumState) const
    {
      return Delegate.get() ? Delegate->GetSoundState(volState, spectrumState) : MODULE_STOPPED;
    }


    /// Rendering frame
    virtual State RenderFrame(const Sound::Parameters& params, Sound::Receiver& receiver)
    {
      return Delegate.get() ? Delegate->RenderFrame(params, receiver) : MODULE_STOPPED;
    }

    /// Controlling
    virtual State Reset()
    {
      return Delegate.get() ? Delegate->Reset() : MODULE_STOPPED;
    }

    virtual State SetPosition(const uint32_t& frame)
    {
      return Delegate.get() ? Delegate->SetPosition(frame) : MODULE_STOPPED;
    }
  private:
    ModulePlayer::Ptr Delegate;
    Module::Information Information;
  };

  //////////////////////////////////////////////////////////////////////////
  void Describing(ModulePlayer::Info& info)
  {
    info.Capabilities = CAP_MULTITRACK;
    info.Properties.clear();
    info.Properties.insert(StringMap::value_type(ATTR_DESCRIPTION, TEXT_TRD_INFO));
    info.Properties.insert(StringMap::value_type(ATTR_VERSION, TEXT_TRD_VERSION));
  }

  //checking top-level container
  bool Checking(const String& /*filename*/, const IO::DataContainer& source)
  {
    const std::size_t limit(source.Size());
    if (limit != TRD_MODULE_SIZE)
    {
      return false;
    }
    const uint8_t* const data(static_cast<const uint8_t*>(source.Data()));
    //TODO
    return true;
  }

  ModulePlayer::Ptr Creating(const String& filename, const IO::DataContainer& data)
  {
    assert(Checking(filename, data) || !"Attempt to create trd player on invalid data");
    return ModulePlayer::Ptr(new PlayerImpl(filename, data));
  }

  PluginAutoRegistrator trdReg(Checking, Creating, Describing);
}
