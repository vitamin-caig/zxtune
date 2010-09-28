/*
Abstract:
  PSG modules playback support

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//local includes
#include "ay_base.h"
#include "ay_conversion.h"
#include <core/plugins/enumerator.h>
#include <core/plugins/players/module_properties.h>
#include <core/plugins/players/streaming.h>
//common includes
#include <tools.h>
#include <logging.h>
//library includes
#include <core/convert_parameters.h>
#include <core/error_codes.h>
#include <core/module_attrs.h>
#include <core/plugin_attrs.h>
#include <io/container.h>
//boost includes
#include <boost/bind.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/make_shared.hpp>
//text includes
#include <core/text/core.h>
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

  class PSGData
  {
  public:
    typedef boost::shared_ptr<PSGData> RWPtr;
    typedef boost::shared_ptr<const PSGData> Ptr;

    std::vector<AYM::DataChunk> Dump;
  };

  Player::Ptr CreatePSGPlayer(Information::Ptr info, PSGData::Ptr data, AYM::Chip::Ptr device);

  class PSGHolder : public Holder
  {
  public:
    PSGHolder(Plugin::Ptr plugin, const MetaContainer& container, ModuleRegion& region)
      : SrcPlugin(plugin)
      , Data(boost::make_shared<PSGData>())
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
      //detect as much chunks as possible, in despite of real format issues
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
            ++size;//put byte back
            break;
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
            ++size;//put byte back
            break;
          }
          chunk->Data[reg] = *bdata;
          chunk->Mask |= uint_t(1) << reg;
          ++bdata;
          --size;
        }
        else
        {
          ++size;//put byte back
          break;
        }
      }

      //fill region
      region.Offset = 0;
      region.Size = data.Size() - size;
      RawData = region.Extract(*container.Data);

      //meta properties
      const ModuleProperties::Ptr props = ModuleProperties::Create(PSG_PLUGIN_ID);
      props->SetSource(RawData, region);
      props->SetPlugins(container.Plugins);
      props->SetPath(container.Path);
      Info = CreateStreamInfo(Data->Dump.size(), AYM::LOGICAL_CHANNELS, AYM::CHANNELS, props);
    }

    virtual Plugin::Ptr GetPlugin() const
    {
      return SrcPlugin;
    }

    virtual Information::Ptr GetModuleInformation() const
    {
      return Info;
    }

    virtual Player::Ptr CreatePlayer() const
    {
      return CreatePSGPlayer(Info, Data, AYM::CreateChip());
    }

    virtual Error Convert(const Conversion::Parameter& param, Dump& dst) const
    {
      using namespace Conversion;
      Error result;
      //converting to PSG and raw ripping are the same
      if (parameter_cast<RawConvertParam>(&param) ||
          parameter_cast<PSGConvertParam>(&param))
      {
        const uint8_t* const data = static_cast<const uint8_t*>(RawData->Data());
        dst.assign(data, data + RawData->Size());
      }
      else if (!ConvertAYMFormat(boost::bind(&CreatePSGPlayer, boost::cref(Info), boost::cref(Data), _1),
        param, dst, result))
      {
        return Error(THIS_LINE, ERROR_MODULE_CONVERT, Text::MODULE_ERROR_CONVERSION_UNSUPPORTED);
      }
      return result;
    }
  private:
    const Plugin::Ptr SrcPlugin;
    const PSGData::RWPtr Data;
    Information::Ptr Info;
    IO::DataContainer::Ptr RawData;
  };

  class PSGDataRenderer : public AYMDataRenderer
  {
  public:
    explicit PSGDataRenderer(PSGData::Ptr data)
      : Data(data)
    {
    }

    virtual void SynthesizeData(const TrackState& state, AYMTrackSynthesizer& synthesizer)
    {
      const AYM::DataChunk& data = Data->Dump[state.Frame()];
      //collect state
      for (uint_t reg = 0, mask = (data.Mask & AYM::DataChunk::MASK_ALL_REGISTERS); mask; ++reg, mask >>= 1)
      {
        if (0 != (mask & 1))
        {
          PlayerState.Data[reg] = data.Data[reg];
          PlayerState.Mask |= uint_t(1) << reg;
        }
      }
      //apply result
      synthesizer.SetRawChunk(PlayerState);
      //reset envelope mask
      PlayerState.Mask &= ~(uint_t(1) << AYM::DataChunk::REG_ENV);
    }

    virtual void Reset()
    {
      PlayerState.Mask = AYM::DataChunk::MASK_ALL_REGISTERS;
      PlayerState.Data[AYM::DataChunk::REG_MIXER] = 0xff;
    }
  private:
    const PSGData::Ptr Data;
    //internal state
    AYM::DataChunk PlayerState;
  };

  Player::Ptr CreatePSGPlayer(Information::Ptr info, PSGData::Ptr data, AYM::Chip::Ptr device)
  {
    const AYMDataRenderer::Ptr renderer = boost::make_shared<PSGDataRenderer>(data);
    return CreateAYMStreamPlayer(info, renderer, device);
  }

  bool CheckPSG(const IO::DataContainer& inputData)
  {
    if (inputData.Size() <= sizeof(PSGHeader))
    {
      return false;
    }
    const PSGHeader* const header = safe_ptr_cast<const PSGHeader*>(inputData.Data());
    return 0 == std::memcmp(header->Sign, PSG_SIGNATURE, sizeof(PSG_SIGNATURE)) &&
       PSG_MARKER == header->Marker;
  }

  class PSGPlugin : public PlayerPlugin
                  , public boost::enable_shared_from_this<PSGPlugin>
  {
  public:
    virtual String Id() const
    {
      return PSG_PLUGIN_ID;
    }

    virtual String Description() const
    {
      return Text::PSG_PLUGIN_INFO;
    }

    virtual String Version() const
    {
      return PSG_PLUGIN_VERSION;
    }

    virtual uint_t Capabilities() const
    {
      return CAP_STOR_MODULE | CAP_DEV_AYM | CAP_CONV_RAW | GetSupportedAYMFormatConvertors();
    }

    virtual bool Check(const IO::DataContainer& inputData) const
    {
      return CheckPSG(inputData);
    }

    virtual Module::Holder::Ptr CreateModule(const Parameters::Accessor& /*parameters*/,
                                             const MetaContainer& container,
                                             ModuleRegion& region) const
    {
      assert(CheckPSG(*container.Data));

      try
      {
        const Plugin::Ptr plugin = shared_from_this();
        const Module::Holder::Ptr holder(new PSGHolder(plugin, container, region));
        return holder;
      }
      catch (const Error&/*e*/)
      {
        Log::Debug("Core::PSGSupp", "Failed to create holder");
      }
      return Module::Holder::Ptr();
    }
  };
}

namespace ZXTune
{
  void RegisterPSGSupport(PluginsEnumerator& enumerator)
  {
    const PlayerPlugin::Ptr plugin(new PSGPlugin());
    enumerator.RegisterPlugin(plugin);
  }
}
