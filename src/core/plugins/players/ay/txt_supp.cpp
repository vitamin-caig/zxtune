/*
Abstract:
  TXT modules playback support

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//local includes
#include "ay_base.h"
#include "ay_conversion.h"
#include "vortex_io.h"
#include "aym_parameters_helper.h"
#include "core/plugins/registrator.h"
#include "core/plugins/players/creation_result.h"
#include "core/plugins/players/module_properties.h"
//common includes
#include <byteorder.h>
#include <error_tools.h>
#include <logging.h>
#include <tools.h>
//library includes
#include <core/convert_parameters.h>
#include <core/core_parameters.h>
#include <core/error_codes.h>
#include <core/module_attrs.h>
#include <core/plugin_attrs.h>
#include <io/container.h>
//std includes
#include <cctype>
//boost includes
#include <boost/enable_shared_from_this.hpp>
#include <boost/algorithm/string.hpp>
//text includes
#include <core/text/core.h>
#include <core/text/plugins.h>
#include <core/text/warnings.h>

#define FILE_TAG 24577A20

namespace
{
  using namespace ZXTune;
  using namespace ZXTune::Module;

  const Char TXT_PLUGIN_ID[] = {'T', 'X', 'T', 0};
  const String TXT_PLUGIN_VERSION(FromStdString("$Rev$"));

  const std::size_t MIN_MODULE_SIZE = 256;
  const std::size_t MAX_MODULE_SIZE = 524288;//512k is more than enough

  ////////////////////////////////////////////
  inline bool CheckSymbol(char sym)
  {
    return !(sym >= ' ' || sym == '\r' || sym == '\n');
  }
  
  class TXTHolder : public Holder
                  , private ConversionFactory
  {
  public:
    TXTHolder(ModuleProperties::Ptr properties, Parameters::Accessor::Ptr parameters, IO::DataContainer::Ptr data, std::size_t& usedSize)
      : Data(Vortex::Track::ModuleData::Create())
      , Properties(properties)
      , Info(CreateTrackInfo(Data, Devices::AYM::CHANNELS))
      , Params(parameters)
    {
      const std::size_t dataSize = data->Size();
      const char* const rawData = static_cast<const char*>(data->Data());
      const char* const dataEnd = std::find_if(rawData, rawData + std::min(MAX_MODULE_SIZE, dataSize), &CheckSymbol);
      const std::size_t limit = dataEnd - rawData;

      ThrowIfError(Vortex::ConvertFromText(std::string(rawData, dataEnd),
        *Data, *Properties, Version));

      usedSize = limit;
      //meta properties
      //TODO: calculate fixed data in ConvertFromText
      Properties->SetSource(usedSize, ModuleRegion(0, usedSize));
    }

    virtual Plugin::Ptr GetPlugin() const
    {
      return Properties->GetPlugin();
    }

    virtual Information::Ptr GetModuleInformation() const
    {
      return Info;
    }

    virtual Parameters::Accessor::Ptr GetModuleProperties() const
    {
      return Parameters::CreateMergedAccessor(Params, Properties);
    }

    virtual Renderer::Ptr CreateRenderer(Sound::MultichannelReceiver::Ptr target) const
    {
      const Parameters::Accessor::Ptr params = GetModuleProperties();

      const AYM::TrackParameters::Ptr trackParams = AYM::TrackParameters::Create(params);
      const Devices::AYM::Receiver::Ptr receiver = CreateAYMReceiver(trackParams, target);
      const Devices::AYM::ChipParameters::Ptr chipParams = AYM::CreateChipParameters(params);
      const Devices::AYM::Chip::Ptr chip = Devices::AYM::CreateChip(chipParams, receiver);
      return Vortex::CreateRenderer(trackParams, Info, Data, Version, chip);
    }

    virtual Error Convert(const Conversion::Parameter& param, Dump& dst) const
    {
      using namespace Conversion;
      Error result;
      if (parameter_cast<RawConvertParam>(&param))
      {
        Properties->GetData(dst);
        return result;
      }
      else if (ConvertAYMFormat(param, *this, dst, result))
      {
        return result;
      }
      else if (ConvertVortexFormat(*Data, *Info, *GetModuleProperties(), param, Version, dst, result))
      {
        return result;
      }
      return Error(THIS_LINE, ERROR_MODULE_CONVERT, Text::MODULE_ERROR_CONVERSION_UNSUPPORTED);
    }
  private:
    virtual Information::Ptr GetInformation() const
    {
      return GetModuleInformation();
    }

    virtual Parameters::Accessor::Ptr GetProperties() const
    {
      return GetModuleProperties();
    }

    virtual Renderer::Ptr CreateRenderer(Devices::AYM::Chip::Ptr chip) const
    {
      const Parameters::Accessor::Ptr params = GetModuleProperties();

      const AYM::TrackParameters::Ptr trackParams = AYM::TrackParameters::Create(params);
      return Vortex::CreateRenderer(trackParams, Info, Data, Version, chip);
    }
  private:
    const Vortex::Track::ModuleData::RWPtr Data;
    const ModuleProperties::Ptr Properties;
    const Information::Ptr Info;
    uint_t Version;
    const Parameters::Accessor::Ptr Params;
  };

  const std::string TXT_FORMAT(
    "'['M'o'd'u'l'e']" //[Module]
  );

  //////////////////////////////////////////////////////////////////////////
  class TXTPlugin : public PlayerPlugin
                  , public ModulesFactory
                  , public boost::enable_shared_from_this<TXTPlugin>
  {
  public:
    typedef boost::shared_ptr<const TXTPlugin> Ptr;

    TXTPlugin()
      : Format(DataFormat::Create(TXT_FORMAT))
    {
    }

    virtual String Id() const
    {
      return TXT_PLUGIN_ID;
    }

    virtual String Description() const
    {
      return Text::TXT_PLUGIN_INFO;
    }

    virtual String Version() const
    {
      return TXT_PLUGIN_VERSION;
    }

    virtual uint_t Capabilities() const
    {
      return CAP_STOR_MODULE | CAP_DEV_AYM | CAP_CONV_RAW |
      GetSupportedAYMFormatConvertors() | GetSupportedVortexFormatConvertors();
    }

    virtual bool Check(const IO::DataContainer& inputData) const
    {
      return inputData.Size() > MIN_MODULE_SIZE && Format->Match(inputData.Data(), inputData.Size());
    }

    virtual DetectionResult::Ptr Detect(DataLocation::Ptr inputData, const Module::DetectCallback& callback) const
    {
      const TXTPlugin::Ptr self = shared_from_this();
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
        const Holder::Ptr holder(new TXTHolder(properties, parameters, data, usedSize));
        return holder;
      }
      catch (const Error& e)
      {
        Log::Debug("Core::TXTSupp", "Failed to create holder ('%1%')", e.GetText());
      }
      return Holder::Ptr();
    }
  private:
    const DataFormat::Ptr Format;
  };
}

namespace ZXTune
{
  void RegisterTXTSupport(PluginsRegistrator& registrator)
  {
    const PlayerPlugin::Ptr plugin(new TXTPlugin());
    registrator.RegisterPlugin(plugin);
  }
}
