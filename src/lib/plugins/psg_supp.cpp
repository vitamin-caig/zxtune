#include "plugin_enumerator.h"
#include "player_base.h"
#include "../devices/data_source.h"
#include "../devices/aym/aym.h"
#include "../io/container.h"

#include <error.h>
#include <tools.h>

#include <sound_attrs.h>
#include <player_attrs.h>
#include <convert_parameters.h>

#include <boost/static_assert.hpp>

#include <cassert>

#define FILE_TAG 59843902

namespace
{
  using namespace ZXTune;

  const String TEXT_PSG_INFO("PSG modules support");
  const String TEXT_PSG_VERSION("0.1");

  //TODO
  const String::value_type TEXT_EMPTY[] = {0};

  const std::size_t MAX_MODULE_SIZE = 65536;

  typedef IO::FastDump<uint8_t> FastDump;

  const uint8_t PSG_SIGNATURE[] = {'P', 'S', 'G'};
  const uint8_t PSG_MARKER = 0x1a;

  const uint8_t INT_BEGIN = 0xff;
  const uint8_t INT_SKIP = 0xfe;
  const uint8_t MUS_END = 0xfd;

#ifdef USE_PRAGMA_PACK
#pragma pack(push,1)
#endif
  PACK_PRE struct PSGHeader
  {
    uint8_t Sign[3];
    uint8_t Marker;
    uint8_t Version;
    uint8_t Interrupt;
    uint8_t Padding[10];
  } PACK_POST;
#ifdef USE_PRAGMA_PACK
#pragma pack(pop)
#endif

  BOOST_STATIC_ASSERT(sizeof(PSGHeader) == 16);

  void Describing(ModulePlayer::Info& info);

  class PlayerImpl : public PlayerBase
  {
  public:
    PlayerImpl(const String& filename, const FastDump& data)
      : PlayerBase(filename)
      , Device(AYM::CreateChip()), CurrentState(MODULE_STOPPED), Filename(filename), TickCount(), Position()
    {
      //workaround for some emulators
      const std::size_t offset = data[4] == INT_BEGIN ? 4 : sizeof(PSGHeader);
      std::size_t size = data.Size() - offset;
      const uint8_t* bdata = &data[offset];
      if (INT_BEGIN != *bdata)
      {
        throw 1;//TODO
      }
      AYM::DataChunk dummy;
      AYM::DataChunk* chunk = &dummy;
      while (size)
      {
        uint8_t reg = *bdata;
        ++bdata;
        --size;
        if (INT_BEGIN == reg)
        {
          Storage.push_back(dummy);
          chunk = &Storage.back();
        }
        else if (INT_SKIP == reg)
        {
          if (size < 1)
          {
            throw 1;//TODO
          }
          std::size_t count = 4 * *bdata;
          while (count--)
          {
            //empty chunk
            Storage.push_back(dummy);
          }
          chunk = &Storage.back();
          ++bdata;
          --size;
        }
        else if (MUS_END == reg)
        {
          break;
        }
        else if (reg <= 15) //register
        {
          if (size < 1)
          {
            throw 1;//TODO
          }
          chunk->Data[reg] = *bdata;
          chunk->Mask |= 1 << reg;
          ++bdata;
          --size;
        }
      }
      const std::size_t rawSize = data.Size() - size;

      Information.Capabilities = 0;
      Information.Properties.clear();
      Information.Loop = 0;
      Information.Statistic.Channels = 3;
      Information.Statistic.Note = 0;
      Information.Statistic.Frame = static_cast<uint32_t>(Storage.size());
      Information.Statistic.Pattern = 0;
      Information.Statistic.Position = 0;
      Information.Statistic.Tempo = 1;

      FillProperties(TEXT_EMPTY, TEXT_EMPTY, TEXT_EMPTY, &data[0], rawSize);
    }

    virtual void GetInfo(Info& info) const
    {
      Describing(info);
    }

    /// Retrieving information about loaded module
    virtual void GetModuleInfo(Module::Information& info) const
    {
      info = Information;
    }

    /// Retrieving current state of loaded module
    virtual State GetModuleState(std::size_t& timeState, Module::Tracking& trackState) const
    {
      timeState = static_cast<uint32_t>(Position);
      trackState.Channels = 3;
      trackState.Note = 0;
      trackState.Pattern = 0;
      trackState.Position = 0;
      trackState.Tempo = 1;
      return CurrentState;
    }

    /// Retrieving current state of sound
    virtual State GetSoundState(Sound::Analyze::ChannelsState& state) const
    {
      assert(Device.get());
      Device->GetState(state);
      return CurrentState;
    }

    /// Rendering frame
    virtual State RenderFrame(const Sound::Parameters& params, Sound::Receiver& receiver)
    {
      if (Position >= Storage.size())
      {
        if (params.Flags & Sound::MOD_LOOP)
        {
          Position = 0;
        }
        else
        {
          receiver.Flush();
          return CurrentState = MODULE_STOPPED;
        }
      }
      assert(Device.get());
      AYM::DataChunk& data(Storage[Position++]);
      data.Tick = (TickCount += params.ClocksPerFrame());
      Device->RenderData(params, data, receiver);
      return CurrentState = MODULE_PLAYING;
    }

    /// Controlling
    virtual State Reset()
    {
      assert(Device.get());
      Position = 0;
      TickCount = 0;
      Device->Reset();
      return CurrentState = MODULE_STOPPED;
    }
    virtual State SetPosition(std::size_t frame)
    {
      if (frame >= Storage.size())
      {
        return Reset();
      }
      assert(Device.get());
      Position = frame;
      TickCount = 0;
      Device->Reset();
      return CurrentState;
    }

    virtual void Convert(const Conversion::Parameter& param, Dump& dst) const
    {
      using namespace Conversion;
      if (const RawConvertParam* const p = parameter_cast<RawConvertParam>(&param))
      {
        dst = RawData;
      }
      else
      {
        throw Error(ERROR_DETAIL, 1);//TODO
      }
    }

  private:
    Module::Information Information;
    AYM::Chip::Ptr Device;
    Dump RawData;
    State CurrentState;
    String Filename;
    uint64_t TickCount;
    std::size_t Position;
    std::vector<AYM::DataChunk> Storage;
  };


  void Describing(ModulePlayer::Info& info)
  {
    info.Capabilities = CAP_DEV_AYM | CAP_CONV_RAW;
    info.Properties.clear();
    info.Properties.insert(StringMap::value_type(ATTR_DESCRIPTION, TEXT_PSG_INFO));
    info.Properties.insert(StringMap::value_type(ATTR_VERSION, TEXT_PSG_VERSION));
  }

  bool Checking(const String& /*filename*/, const IO::DataContainer& data, uint32_t /*capFilter*/)
  {
    if (data.Size() <= sizeof(PSGHeader)/* || data.Size() > MAX_MODULE_SIZE*/)
    {
      return false;
    }
    const PSGHeader* header(safe_ptr_cast<const PSGHeader*>(data.Data()));
    return (0 == std::memcmp(header->Sign, PSG_SIGNATURE, sizeof(PSG_SIGNATURE)) && PSG_MARKER == header->Marker);
  }

  ModulePlayer::Ptr Creating(const String& filename, const IO::DataContainer& data, uint32_t /*capFilter*/)
  {
    assert(Checking(filename, data, 0) || !"Attempt to create player on invalid data");
    return ModulePlayer::Ptr(new PlayerImpl(filename, FastDump(data)));
  }

  PluginAutoRegistrator registrator(Checking, Creating, Describing);
}
