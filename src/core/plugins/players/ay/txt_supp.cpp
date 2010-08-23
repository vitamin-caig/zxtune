/*
Abstract:
  TXT modules playback support

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//local includes
#include "ay_conversion.h"
#include "vortex_io.h"
#include "aym_parameters_helper.h"
#include <core/plugins/enumerator.h>
//common includes
#include <byteorder.h>
#include <error_tools.h>
#include <logging.h>
#include <messages_collector.h>
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
#include <boost/bind.hpp>
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

  const char TXT_MODULE_ID[] = {'[', 'M', 'o', 'd', 'u', 'l', 'e', ']'};

  ////////////////////////////////////////////
  class TXTHolder : public Holder
  {
  public:
    //region must be filled
    TXTHolder(Plugin::Ptr plugin, const MetaContainer& container, const ModuleRegion& region)
      : SrcPlugin(plugin)
      , Data(Vortex::Track::ModuleData::Create())
    {
      const char* const dataIt = static_cast<const char*>(container.Data->Data());

      ThrowIfError(Vortex::ConvertFromText(std::string(dataIt, dataIt + region.Size),
        *Data, Version, FreqTableName));
      //meta properties
      ExtractMetaProperties(TXT_PLUGIN_ID, container, region, region, Data->Info.Properties, RawData);
    }

    virtual const Plugin& GetPlugin() const
    {
      return *SrcPlugin;
    }

    virtual void GetModuleInformation(Information& info) const
    {
      info = Data->Info;
    }

    virtual Player::Ptr CreatePlayer() const
    {
      return Vortex::CreatePlayer(Data, Version, FreqTableName, AYM::CreateChip());
    }

    virtual Error Convert(const Conversion::Parameter& param, Dump& dst) const
    {
      using namespace Conversion;
      Error result;
      if (parameter_cast<RawConvertParam>(&param))
      {
        dst = RawData;
        return Error();
      }
      else if (ConvertAYMFormat(boost::bind(&Vortex::CreatePlayer, boost::cref(Data), Version, FreqTableName, _1),
        param, dst, result))
      {
        return result;
      }
      else if (ConvertVortexFormat(*Data, param, Version, FreqTableName, dst, result))
      {
        return result;
      }
      return Error(THIS_LINE, ERROR_MODULE_CONVERT, Text::MODULE_ERROR_CONVERSION_UNSUPPORTED);
    }
  private:
    const Plugin::Ptr SrcPlugin;
    Dump RawData;
    const Vortex::Track::ModuleData::Ptr Data;
    uint_t Version;
    String FreqTableName;
  };

  //////////////////////////////////////////////////////////////////////////
  inline bool CheckSymbol(char sym)
  {
    return !(sym >= ' ' || sym == '\r' || sym == '\n');
  }

  class TXTPlugin : public PlayerPlugin
                  , public boost::enable_shared_from_this<TXTPlugin>
  {
  public:
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

    virtual Module::Holder::Ptr CreateModule(const Parameters::Map& /*parameters*/,
                                             const MetaContainer& container,
                                             ModuleRegion& region) const
    {
      const std::size_t dataSize = container.Data->Size();
      const char* const data = static_cast<const char*>(container.Data->Data());
      if (dataSize < sizeof(TXT_MODULE_ID) ||
          0 != std::memcmp(data, TXT_MODULE_ID, sizeof(TXT_MODULE_ID)))
      {
        return Module::Holder::Ptr();
      }
      
      const char* const dataEnd = std::find_if(data, data + std::min(MAX_MODULE_SIZE, dataSize), CheckSymbol);
      const std::size_t limit = dataEnd - data;

      if (limit < MIN_MODULE_SIZE)
      {
        return Module::Holder::Ptr();
      }

      ModuleRegion tmpRegion;
      tmpRegion.Size = limit;
      //try to create holder
      try
      {
        const Plugin::Ptr plugin = shared_from_this();
        const Module::Holder::Ptr holder(new TXTHolder(plugin, container, tmpRegion));
    #ifdef SELF_TEST
        holder->CreatePlayer();
    #endif
        region = tmpRegion;
        return holder;
      }
      catch (const Error& e)
      {
        Log::Debug("TXTSupp", "Failed to create holder ('%1%')", e.GetText());
      }
      return Module::Holder::Ptr();
    }
  };
}

namespace ZXTune
{
  void RegisterTXTSupport(PluginsEnumerator& enumerator)
  {
    const PlayerPlugin::Ptr plugin(new TXTPlugin());
    enumerator.RegisterPlugin(plugin);
  }
}
