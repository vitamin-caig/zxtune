/*
Abstract:
  PSG modules playback support

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#include "../enumerator.h"

#include <tools.h>

#include <core/plugin.h>
#include <core/plugin_attrs.h>
#include <core/module_attrs.h>
#include <core/module_types.h>
#include <core/error_codes.h>
#include <core/devices/aym/aym.h>
#include <core/convert_parameters.h>

#include <sound/receiver.h>
#include <sound/sound_attrs.h>
#include <sound/sound_params.h>

#include <io/container.h>

#include <text/core.h>
#include <text/plugins.h>

#define FILE_TAG 59843902

namespace
{
  using namespace ZXTune;
  using namespace ZXTune::Module;

  const String TEXT_PSG_VERSION(FromChar("Revision: $Rev$"));

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

  inline void MakeDump(const IO::FastDump& data, const ModuleRegion& region, Dump& dst)
  {
    dst = Dump(data.Data() + region.Offset, data.Data() + region.Offset + region.Size);
  }
  
  
  void DescribePlugin(PluginInformation& info)
  {
    info.Capabilities = CAP_DEV_AYM | CAP_CONV_RAW;
    info.Properties.clear();
    info.Properties.insert(ParametersMap::value_type(ATTR_DESCRIPTION, TEXT_PSG_INFO));
    info.Properties.insert(ParametersMap::value_type(ATTR_VERSION, TEXT_PSG_VERSION));
  }  
  
  class PSGPlayer : public Player
  {
  public:
    PSGPlayer(const String& filename, const IO::FastDump& data, ModuleRegion& region)
      : Device(AYM::CreateChip()), CurrentState(MODULE_STOPPED), TickCount(), Position()
    {
      //workaround for some emulators
      const std::size_t offset = (data[4] == INT_BEGIN) ? 4 : sizeof(PSGHeader);
      std::size_t size = data.Size() - offset;
      const uint8_t* bdata = &data[offset];
      if (INT_BEGIN != *bdata)
      {
        throw Error(THIS_LINE, ERROR_INVALID_FORMAT, TEXT_MODULE_ERROR_INVALID_FORMAT);
      }
      AYM::DataChunk dummy;
      AYM::DataChunk* chunk = &dummy;
      while (size)
      {
        unsigned reg = *bdata;
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
            throw Error(THIS_LINE, ERROR_INVALID_FORMAT, TEXT_MODULE_ERROR_INVALID_FORMAT);
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
            throw Error(THIS_LINE, ERROR_INVALID_FORMAT, TEXT_MODULE_ERROR_INVALID_FORMAT);
          }
          chunk->Data[reg] = *bdata;
          chunk->Mask |= 1 << reg;
          ++bdata;
          --size;
        }
      }
      
      //fill region
      region.Offset = 0;
      region.Size = data.Size() - size;
      
      //copy raw dump
      MakeDump(data, region, RawData);
      
      //fill properties
      Information.Statistic.Frame = Storage.size();
      Information.Statistic.Tempo = 1;
      Information.Statistic.Channels = 3;
      Information.PhysicalChannels = 3;
      Information.Properties.insert(ParametersMap::value_type(ATTR_FILENAME, filename));
    }

    virtual void GetPlayerInfo(PluginInformation& info) const
    {
      DescribePlugin(info);
    }

    virtual void GetModuleInformation(Information& info) const
    {
      info = Information;
    }

    virtual Error GetModuleState(unsigned& timeState,
                                 Tracking& trackState,
                                 Analyze::ChannelsState& analyzeState) const
    {
      timeState = static_cast<uint32_t>(Position);
      trackState = Information.Statistic;
      trackState.Line = timeState;
      Device->GetState(analyzeState);
      return Error();
    }

    virtual Error RenderFrame(const Sound::RenderParameters& params,
                              PlaybackState& state,
                              Sound::MultichannelReceiver& receiver)
    {
      if (Position >= Storage.size())
      {
        if (MODULE_STOPPED == CurrentState)
        {
          return Error(THIS_LINE, ERROR_MODULE_END, TEXT_MODULE_ERROR_MODULE_END);
        }
        if (params.Flags & Sound::MOD_LOOP)
        {
          Position = 0;
        }
        else
        {
          receiver.Flush();
          state = CurrentState = MODULE_STOPPED;
          return Error();
        }
      }
      assert(Device.get());
      AYM::DataChunk& data(Storage[Position++]);
      data.Tick = (TickCount += params.ClocksPerFrame());
      Device->RenderData(params, data, receiver);
      state = CurrentState = MODULE_PLAYING;
      return Error();
    }

    virtual Error Reset()
    {
      assert(Device.get());
      Position = 0;
      TickCount = 0;
      Device->Reset();
      CurrentState = MODULE_STOPPED;
      return Error();
    }
    
    virtual Error SetPosition(unsigned frame)
    {
      if (frame >= Storage.size())
      {
        return Error(THIS_LINE, ERROR_MODULE_OVERSEEK, TEXT_MODULE_ERROR_OVERSEEK);
      }
      assert(Device.get());
      Position = frame;
      TickCount = 0;
      Device->Reset();
      return Error();
    }

    virtual Error Convert(const Conversion::Parameter& param, Dump& dst) const
    {
      using namespace Conversion;
      if (const RawConvertParam* const p = parameter_cast<RawConvertParam>(&param))
      {
        dst = RawData;
        return Error();
      }
      else
      {
        return Error(THIS_LINE, ERROR_MODULE_CONVERT, TEXT_MODULE_ERROR_CONVERSION_UNSUPPORTED);
      }
    }

  private:
    Module::Information Information;
    AYM::Chip::Ptr Device;
    Dump RawData;
    PlaybackState CurrentState;
    uint64_t TickCount;
    std::size_t Position;
    std::vector<AYM::DataChunk> Storage;
  };
  
  bool CreatePlayer(const String& filename, const IO::FastDump& data, ModuleRegion& region, Player::Ptr& player)
  {
    //perform fast check
    if (data.Size() <= sizeof(PSGHeader))
    {
      return false;
    }
    const PSGHeader* const header(safe_ptr_cast<const PSGHeader*>(data.Data()));
    if (0 == std::memcmp(header->Sign, PSG_SIGNATURE, sizeof(PSG_SIGNATURE)) && 
        PSG_MARKER == header->Marker)
    {
      try
      {
        player.reset(new PSGPlayer(filename, data, region));
        return true;
      }
      catch (const Error&/*e*/)
      {
        //TODO: log error
      }
    }
    return false;
  }
}

namespace ZXTune
{
  void RegisterPSGPlayer(PluginsEnumerator& enumerator)
  {
    PluginInformation info;
    DescribePlugin(info);
    enumerator.RegisterPlayerPlugin(info, CreatePlayer);
  }
}
