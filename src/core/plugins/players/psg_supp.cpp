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
  
  class PSGData
  {
  public:
    typedef boost::shared_ptr<PSGData> Ptr;
    typedef boost::shared_ptr<const PSGData> ConstPtr;

    void FillStatisticInfo()
    {
      Info.LogicalChannels = Info.PhysicalChannels = AYM::CHANNELS;
      Info.FramesCount = Dump.size();
    }

    void InitState(State& state) const
    {
      state = State();
      Tracking& trackRef(state.Reference);
      trackRef.Quirk = 1;
      trackRef.Frame = Info.FramesCount;
      trackRef.Channels = Info.LogicalChannels;
    }

    bool UpdateState(State& state, Sound::LoopMode loopMode) const
    {
      //update tick outside
      ++state.Frame;
      if (++state.Track.Frame >= state.Reference.Frame)
      {
        //check if looped
        state.Track.Frame = 0;
        return Sound::LOOP_NONE != loopMode;
      }
      return true;
    }

    std::vector<AYM::DataChunk> Dump;
    Information Info;
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
          Data->Dump.push_back(dummy);
          chunk = &Data->Dump.back();
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
            Data->Dump.push_back(dummy);
          }
          chunk = &Data->Dump.back();
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
      ExtractMetaProperties(PSG_PLUGIN_ID, container, region, region, Data->Info.Properties, RawData);
      Data->FillStatisticInfo();
    }

    virtual void GetPluginInformation(PluginInformation& info) const
    {
      DescribePSGPlugin(info);
    }

    virtual void GetModuleInformation(Information& info) const
    {
      info = Data->Info;
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
    Dump RawData;
    PSGData::Ptr Data;
  };

  typedef AYMPlayer<PSGData, AYM::DataChunk> PSGPlayerBase;

  class PSGPlayer : public PSGPlayerBase
  {
  public:
    PSGPlayer(PSGData::ConstPtr data, AYM::Chip::Ptr device)
       : PSGPlayerBase(data, device, TABLE_SOUNDTRACKER/*any of*/)
    {
    }

    virtual Error Reset()
    {
      const Error& res = PSGPlayerBase::Reset();
      PlayerState.Mask = AYM::DataChunk::MASK_ALL_REGISTERS;
      PlayerState.Data[AYM::DataChunk::REG_MIXER] = 0xff;
      return res;
    }
    
    void RenderData(AYM::DataChunk& chunk)
    {
      const AYM::DataChunk& data = Data->Dump[ModState.Track.Frame];
      //collect state
      for (uint_t reg = 0, mask = (data.Mask & AYM::DataChunk::MASK_ALL_REGISTERS); mask; ++reg, mask >>= 1)
      {
        if (0 != (mask & 1))
        {
          PlayerState.Data[reg] = data.Data[reg];
          PlayerState.Mask |= uint_t(1) << reg;
        }
      }
      //copy result
      std::copy(PlayerState.Data.begin(), PlayerState.Data.begin() + AYM::DataChunk::REG_ENV + 1, chunk.Data.begin());
      chunk.Mask &= ~AYM::DataChunk::MASK_ALL_REGISTERS;
      chunk.Mask |= PlayerState.Mask & AYM::DataChunk::MASK_ALL_REGISTERS;
      //reset envelope mask
      PlayerState.Mask &= ~(uint_t(1) << AYM::DataChunk::REG_ENV);
      //count enabled channels
      ModState.Track.Channels = 0;
      for (uint_t chan = 0,
           mixer = PlayerState.Data[AYM::DataChunk::REG_MIXER],
           mask = AYM::DataChunk::REG_MASK_NOISEA | AYM::DataChunk::REG_MASK_TONEA; 
        chan < AYM::CHANNELS; ++chan, mask <<= 1)
      {
        if (0 != (mixer & mask) || 
            0 != (PlayerState.Data[AYM::DataChunk::REG_VOLA + chan] & AYM::DataChunk::REG_MASK_ENV))
        {
          ++ModState.Track.Channels;
        }
      }
    }
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
