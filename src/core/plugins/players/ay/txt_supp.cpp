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

    virtual Holder::Ptr CreateModule(ModuleProperties::RWPtr properties, Parameters::Accessor::Ptr parameters, IO::DataContainer::Ptr allData, std::size_t& usedSize) const
    {
      try
      {
        assert(Check(*allData));

        const std::size_t dataSize = allData->Size();
        const char* const rawData = static_cast<const char*>(allData->Data());
        const char* const dataEnd = std::find_if(rawData, rawData + std::min(MAX_MODULE_SIZE, dataSize), &CheckSymbol);
        const std::size_t limit = dataEnd - rawData;

        const Vortex::Track::ModuleData::RWPtr moduleData = Vortex::Track::ModuleData::Create();

        ThrowIfError(Vortex::ConvertFromText(std::string(rawData, dataEnd),
          *moduleData, *properties));

        usedSize = limit;
        //TODO: calculate fixed data in ConvertFromText
        properties->SetSource(usedSize, ModuleRegion(0, usedSize));

        const AYM::Chiptune::Ptr chiptune = Vortex::CreateChiptune(moduleData, properties, Devices::AYM::CHANNELS);
        const Holder::Ptr nativeHolder = AYM::CreateHolder(chiptune, parameters);

        return Vortex::CreateHolder(moduleData, nativeHolder);
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
