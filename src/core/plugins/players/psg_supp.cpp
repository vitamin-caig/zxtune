/*
Abstract:
  PSG modules playback support

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//local includes
#include "aym_base.h"
#include "convert_helpers.h"
#include <core/plugins/enumerator.h>
//common includes
#include <tools.h>
#include <logging.h>
//library includes
#include <core/convert_parameters.h>
#include <core/module_attrs.h>
#include <core/plugin_attrs.h>
#include <io/container.h>
//boost includes
#include <boost/bind.hpp>
#include <boost/make_shared.hpp>
//text includes
#include <core/text/plugins.h>

#define FILE_TAG 59843902

namespace
{
  using namespace ZXTune;
  using namespace ZXTune::Module;

  const Char PSG_PLUGIN_ID[] = {'P', 'S', 'G', 0};
  const String PSG_PLUGIN_VERSION(FromStdString("$Rev$"));

  const uint8_t PSG_SIGNATURE[] = {'P', 'S', 'G'};
  
  enum
  {
    PSG_MARKER = 0x1a,

    INT_BEGIN = 0xff,
    INT_SKIP = 0xfe,
    MUS_END = 0xfd
  };

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

  void DescribePSGPlugin(PluginInformation& info)
  {
    info.Id = PSG_PLUGIN_ID;
    info.Description = Text::PSG_PLUGIN_INFO;
    info.Version = PSG_PLUGIN_VERSION;
    info.Capabilities = CAP_STOR_MODULE | CAP_DEV_AYM | CAP_CONV_RAW | GetSupportedAYMFormatConvertors();
  }
  
  class PSGData : public std::vector<AYM::DataChunk>
  {
  public:
    typedef boost::shared_ptr<PSGData> Ptr;
    typedef boost::shared_ptr<const PSGData> ConstPtr;
  };

  
  Player::Ptr CreatePSGPlayer(PSGData::ConstPtr data, AYM::Chip::Ptr device);
  
  class PSGHolder : public Holder
  {
  public:
    PSGHolder(const MetaContainer& container, ModuleRegion& region)
      : Data(boost::make_shared<PSGData>())
    {
      const IO::FastDump data = *container.Data;
      //workaround for some emulators
      const std::size_t offset = (data[4] == INT_BEGIN) ? 4 : sizeof(PSGHeader);
      std::size_t size = data.Size() - offset;
      const uint8_t* bdata = &data[offset];
      if (INT_BEGIN != *bdata)
      {
        throw Error(THIS_LINE, ERROR_INVALID_FORMAT);
      }
      AYM::DataChunk dummy;
      AYM::DataChunk* chunk = &dummy;
      while (size)
      {
        const uint_t reg = *bdata;
        ++bdata;
        --size;
        if (INT_BEGIN == reg)
        {
          Data->push_back(dummy);
          chunk = &Data->back();
        }
        else if (INT_SKIP == reg)
        {
          if (size < 1)
          {
            throw Error(THIS_LINE, ERROR_INVALID_FORMAT);
          }
          std::size_t count = 4 * *bdata;
          while (count--)
          {
            //empty chunk
            Data->push_back(dummy);
          }
          chunk = &Data->back();
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
            throw Error(THIS_LINE, ERROR_INVALID_FORMAT);
          }
          chunk->Data[reg] = *bdata;
          chunk->Mask |= uint_t(1) << reg;
          ++bdata;
          --size;
        }
      }
      
      //fill region
      region.Offset = 0;
      region.Size = data.Size() - size;
      
      //extract properties
      ExtractMetaProperties(PSG_PLUGIN_ID, container, region, region, ModInfo.Properties, RawData);
      
      //fill properties
      ModInfo.Statistic.Frame = Data->size();
      ModInfo.Statistic.Tempo = 1;
      ModInfo.Statistic.Channels = AYM::CHANNELS;
      ModInfo.PhysicalChannels = AYM::CHANNELS;
    }

    virtual void GetPluginInformation(PluginInformation& info) const
    {
      DescribePSGPlugin(info);
    }

    virtual void GetModuleInformation(Information& info) const
    {
      info = ModInfo;
    }

    virtual void ModifyCustomAttributes(const Parameters::Map& attrs, bool replaceExisting)
    {
      return Parameters::MergeMaps(ModInfo.Properties, attrs, ModInfo.Properties, replaceExisting);
    }

    virtual Player::Ptr CreatePlayer() const
    {
      return CreatePSGPlayer(Data, AYM::CreateChip());
    }
    
    virtual Error Convert(const Conversion::Parameter& param, Dump& dst) const
    {
      using namespace Conversion;
      Error result;
      //converting to PSG and raw ripping are the same
      if (parameter_cast<RawConvertParam>(&param) ||
          parameter_cast<PSGConvertParam>(&param))
      {
        dst = RawData;
      }
      else if (!ConvertAYMFormat(boost::bind(&CreatePSGPlayer, boost::cref(Data), _1),
        param, dst, result))
      {
        return Error(THIS_LINE, ERROR_MODULE_CONVERT, Text::MODULE_ERROR_CONVERSION_UNSUPPORTED);
      }
      return result;
    }
  private:
    Module::Information ModInfo;
    Dump RawData;
    PSGData::Ptr Data;
  };

  class PSGPlayer : public AYMPlayerBase
  {
  public:
    PSGPlayer(PSGData::ConstPtr data, AYM::Chip::Ptr device)
       : AYMPlayerBase(device, TABLE_SOUNDTRACKER/*any of*/)
       , Data(data)
       , Position(Data->begin())
    {
      Reset();
    }

    virtual Error RenderFrame(const Sound::RenderParameters& params,
                              PlaybackState& state,
                              Sound::MultichannelReceiver& receiver)
    {
      if (Data->end() == Position)
      {
        if (MODULE_STOPPED == CurrentState)
        {
          return Error(THIS_LINE, ERROR_MODULE_END, Text::MODULE_ERROR_MODULE_END);
        }
        receiver.Flush();
        state = CurrentState = MODULE_STOPPED;
        return Error();
      }
      AYM::DataChunk chunk;
      AYMHelper->GetDataChunk(chunk);
      ModState.Tick += params.ClocksPerFrame();
      chunk.Tick = ModState.Tick;
      RenderData(chunk);

      Device->RenderData(params, chunk, receiver);
      //reset envelope register mask
      State.Mask &= ~(uint_t(1) << AYM::DataChunk::REG_ENV);
      if (UpdateState(params.Looping))
      {
        CurrentState = MODULE_PLAYING;
      }
      else
      {
        receiver.Flush();
        CurrentState = MODULE_STOPPED;
      }
      state = CurrentState;
      return Error();
    }

    virtual Error Reset()
    {
      Position = Data->begin();
      ModState = Module::Timing();
      State = AYM::DataChunk();
      State.Mask = AYM::DataChunk::MASK_ALL_REGISTERS;
      State.Data[AYM::DataChunk::REG_MIXER] = 0xff;
      Device->Reset();
      CurrentState = MODULE_STOPPED;
      return Error();
    }
    
    virtual Error SetPosition(uint_t frame)
    {
      if (frame < ModState.Frame)
      {
        //reset to beginning in case of moving back
        const uint64_t keepTicks = ModState.Tick;
        Position = Data->begin();
        ModState = Module::Timing();
        ModState.Tick = keepTicks;
      }
      //fast forward
      AYM::DataChunk chunk;
      while (ModState.Frame < frame)
      {
        //do not update tick for proper rendering
        RenderData(chunk);
        if (!UpdateState(Sound::LOOP_NONE))
        {
          break;
        }
      }
      return Error();
    }
  private:
    void RenderData(AYM::DataChunk& chunk)
    {
      const AYM::DataChunk& data = *Position;
      //collect state
      for (uint_t reg = 0, mask = (data.Mask & AYM::DataChunk::MASK_ALL_REGISTERS); mask; ++reg, mask >>= 1)
      {
        if (0 != (mask & 1))
        {
          State.Data[reg] = data.Data[reg];
          State.Mask |= uint_t(1) << reg;
        }
      }
      //copy result
      std::copy(State.Data.begin(), State.Data.begin() + AYM::DataChunk::REG_ENV + 1, chunk.Data.begin());
      chunk.Mask = (chunk.Mask & ~AYM::DataChunk::MASK_ALL_REGISTERS) | (State.Mask & AYM::DataChunk::MASK_ALL_REGISTERS);
    }

    bool UpdateState(Sound::LoopMode mode)
    {
      ++ModState.Frame;
      if (++Position == Data->end())
      {
        if (Sound::LOOP_NONE == mode)
        {
          return false;
        }
        Position = Data->begin();
        ModState.Frame = 0;
      }
      return true;
    }
  private:
    const PSGData::ConstPtr Data;
    PSGData::const_iterator Position;
    AYM::DataChunk State;
  };
  
  Player::Ptr CreatePSGPlayer(PSGData::ConstPtr data, AYM::Chip::Ptr device)
  {
    return Player::Ptr(new PSGPlayer(data, device));
  }
  
  bool CreatePSGModule(const Parameters::Map& /*commonParams*/, const MetaContainer& container,
    Holder::Ptr& holder, ModuleRegion& region)
  {
    //perform fast check
    const IO::DataContainer& data = *container.Data;
    if (data.Size() <= sizeof(PSGHeader))
    {
      return false;
    }
    const PSGHeader* const header = safe_ptr_cast<const PSGHeader*>(data.Data());
    if (0 == std::memcmp(header->Sign, PSG_SIGNATURE, sizeof(PSG_SIGNATURE)) &&
        PSG_MARKER == header->Marker)
    {
      try
      {
        holder.reset(new PSGHolder(container, region));
        return true;
      }
      catch (const Error&/*e*/)
      {
        Log::Debug("Core::PSGSupp", "Failed to create holder");
      }
    }
    return false;
  }
}

namespace ZXTune
{
  void RegisterPSGSupport(PluginsEnumerator& enumerator)
  {
    PluginInformation info;
    DescribePSGPlugin(info);
    enumerator.RegisterPlayerPlugin(info, CreatePSGModule);
  }
}
