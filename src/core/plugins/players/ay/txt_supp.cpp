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
#include <debug_log.h>
#include <error_tools.h>
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
#include <boost/algorithm/string.hpp>
//text includes
#include <core/text/plugins.h>

#define FILE_TAG 24577A20

namespace
{
  using namespace ZXTune;
  using namespace ZXTune::Module;

  const std::size_t MIN_MODULE_SIZE = 256;
  const std::size_t MAX_MODULE_SIZE = 524288;//512k is more than enough

  ////////////////////////////////////////////
  inline bool CheckSymbol(char sym)
  {
    return !(sym >= ' ' || sym == '\r' || sym == '\n');
  }

  const Debug::Stream Dbg("Core::TXTSupp");
}

namespace
{
  using namespace ZXTune;

  //plugin attributes
  const Char ID[] = {'T', 'X', 'T', 0};
  const Char* const INFO = Text::TXT_PLUGIN_INFO;
  const uint_t CAPS = CAP_STOR_MODULE | CAP_DEV_AYM | CAP_CONV_RAW | GetSupportedAYMFormatConvertors() | GetSupportedVortexFormatConvertors();


  const std::string TXT_FORMAT(
    "'['M'o'd'u'l'e']" //[Module]
  );

  //////////////////////////////////////////////////////////////////////////
  class TXTModulesFactory : public ModulesFactory
  {
  public:
    TXTModulesFactory()
      : Format(Binary::Format::Create(TXT_FORMAT))
    {
    }

    virtual bool Check(const Binary::Container& inputData) const
    {
      return inputData.Size() > MIN_MODULE_SIZE && Format->Match(inputData.Data(), inputData.Size());
    }

    virtual Binary::Format::Ptr GetFormat() const
    {
      return Format;
    }

    virtual Holder::Ptr CreateModule(ModuleProperties::RWPtr properties, Binary::Container::Ptr allData, std::size_t& usedSize) const
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
        const Holder::Ptr nativeHolder = AYM::CreateHolder(chiptune);

        return Vortex::CreateHolder(moduleData, nativeHolder);
      }
      catch (const Error& e)
      {
        Dbg("Failed to create holder ('%1%')", e.GetText());
      }
      return Holder::Ptr();
    }
  private:
    const Binary::Format::Ptr Format;
  };
}

namespace ZXTune
{
  void RegisterTXTSupport(PluginsRegistrator& registrator)
  {
    const ModulesFactory::Ptr factory = boost::make_shared<TXTModulesFactory>();
    const PlayerPlugin::Ptr plugin = CreatePlayerPlugin(ID, INFO, CAPS, factory);
    registrator.RegisterPlugin(plugin);
  }
}
