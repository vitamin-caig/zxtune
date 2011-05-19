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
#include "core/plugins/registrator.h"
#include "core/plugins/players/creation_result.h"
#include "core/plugins/players/module_properties.h"
#include "core/plugins/players/streaming.h"
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

    std::vector<Devices::AYM::DataChunk> Dump;
  };

  Renderer::Ptr CreatePSGRenderer(Information::Ptr info, PSGData::Ptr data, Devices::AYM::Chip::Ptr device);

  class PSGHolder : public Holder
                  , private ConversionFactory
  {
  public:
    PSGHolder(ModuleProperties::Ptr properties, Parameters::Accessor::Ptr parameters, IO::DataContainer::Ptr rawData, std::size_t& usedSize)
      : Properties(properties)
      , Data(boost::make_shared<PSGData>())
    {
      const IO::FastDump data(*rawData);
      //workaround for some emulators
      const std::size_t offset = (data[4] == INT_BEGIN) ? 4 : sizeof(PSGHeader);
      std::size_t size = data.Size() - offset;
      const uint8_t* bdata = &data[offset];
      //detect as much chunks as possible, in despite of real format issues
      Devices::AYM::DataChunk dummy;
      Devices::AYM::DataChunk* chunk = &dummy;
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

      usedSize = data.Size() - size;

      //meta properties
      Properties->SetSource(usedSize, ModuleRegion(0, usedSize));
      Info = CreateStreamInfo(static_cast<uint_t>(Data->Dump.size()), Devices::AYM::CHANNELS, 
        Parameters::CreateMergedAccessor(parameters, Properties));
    }

    virtual Plugin::Ptr GetPlugin() const
    {
      return Properties->GetPlugin();
    }

    virtual Information::Ptr GetModuleInformation() const
    {
      return Info;
    }

    virtual Renderer::Ptr CreateRenderer(Sound::MultichannelReceiver::Ptr target) const
    {
      const Devices::AYM::Receiver::Ptr receiver = CreateAYMReceiver(target);
      return CreatePSGRenderer(Info, Data, Devices::AYM::CreateChip(receiver));
    }

    virtual Error Convert(const Conversion::Parameter& param, Dump& dst) const
    {
      using namespace Conversion;
      Error result;
      //converting to PSG and raw ripping are the same
      if (parameter_cast<RawConvertParam>(&param))
      {
        Properties->GetData(dst);
      }
      else if (!ConvertAYMFormat(param, *this, dst, result))
      {
        return Error(THIS_LINE, ERROR_MODULE_CONVERT, Text::MODULE_ERROR_CONVERSION_UNSUPPORTED);
      }
      return result;
    }
  private:
    virtual Information::Ptr GetInformation() const
    {
      return Info;
    }

    virtual Renderer::Ptr CreateRenderer(Devices::AYM::Chip::Ptr chip) const
    {
      return CreatePSGRenderer(Info, Data, chip);
    }
  private:
    const ModuleProperties::Ptr Properties;
    const PSGData::RWPtr Data;
    Information::Ptr Info;
  };

  class PSGDataRenderer : public AYMDataRenderer
  {
  public:
    explicit PSGDataRenderer(PSGData::Ptr data)
      : Data(data)
    {
    }

    virtual void SynthesizeData(const TrackState& state, const AYM::TrackBuilder& track)
    {
      const Devices::AYM::DataChunk& data = Data->Dump[state.Frame()];
      //collect state
      for (uint_t reg = 0, mask = (data.Mask & Devices::AYM::DataChunk::MASK_ALL_REGISTERS); mask; ++reg, mask >>= 1)
      {
        if (0 != (mask & 1))
        {
          PlayerState.Data[reg] = data.Data[reg];
          PlayerState.Mask |= uint_t(1) << reg;
        }
      }
      //apply result
      track.SetRawChunk(PlayerState);
      //reset envelope mask
      PlayerState.Mask &= ~(uint_t(1) << Devices::AYM::DataChunk::REG_ENV);
    }

    virtual void Reset()
    {
      PlayerState = Devices::AYM::DataChunk();
    }
  private:
    const PSGData::Ptr Data;
    //internal state
    Devices::AYM::DataChunk PlayerState;
  };

  Renderer::Ptr CreatePSGRenderer(Information::Ptr info, PSGData::Ptr data, Devices::AYM::Chip::Ptr device)
  {
    const AYMDataRenderer::Ptr renderer = boost::make_shared<PSGDataRenderer>(data);
    return CreateAYMStreamRenderer(info, renderer, device);
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

  const std::string PSG_FORMAT(
    "'P'S'G" // uint8_t Sign[3];
  );

  class PSGPlugin : public PlayerPlugin
                  , public ModulesFactory
                  , public boost::enable_shared_from_this<PSGPlugin>
  {
  public:
    typedef boost::shared_ptr<const PSGPlugin> Ptr;
    
    PSGPlugin()
      : Format(DataFormat::Create(PSG_FORMAT))
    {
    }

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

    virtual DetectionResult::Ptr Detect(DataLocation::Ptr inputData, const Module::DetectCallback& callback) const
    {
      const PSGPlugin::Ptr self = shared_from_this();
      return DetectModuleInLocation(self, self, inputData, callback);
    }
  private:
    virtual DataFormat::Ptr GetFormat() const
    {
      return Format;
    }

    virtual Holder::Ptr CreateModule(ModuleProperties::Ptr properties, Parameters::Accessor::Ptr parameters, IO::DataContainer::Ptr data, std::size_t& usedSize) const
    {
      try
      {
        assert(Check(*data));
        const Holder::Ptr holder(new PSGHolder(properties, parameters, data, usedSize));
        return holder;
      }
      catch (const Error&/*e*/)
      {
        Log::Debug("Core::PSGSupp", "Failed to create holder");
      }
      return Holder::Ptr();
    }
  private:
    const DataFormat::Ptr Format;
  };
}

namespace ZXTune
{
  void RegisterPSGSupport(PluginsRegistrator& registrator)
  {
    const PlayerPlugin::Ptr plugin(new PSGPlugin());
    registrator.RegisterPlugin(plugin);
  }
}
