/*
Abstract:
  TXT modules playback support

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#include "../enumerator.h"
#include "convert_helpers.h"
#include "vortex_io.h"

#include <byteorder.h>
#include <error_tools.h>
#include <logging.h>
#include <messages_collector.h>
#include <tools.h>
#include <core/convert_parameters.h>
#include <core/core_parameters.h>
#include <core/error_codes.h>
#include <core/module_attrs.h>
#include <core/plugin_attrs.h>
#include <io/container.h>
#include <core/devices/aym_parameters_helper.h>

#include <cctype>

#include <boost/bind.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/algorithm/string.hpp>

#include <text/core.h>
#include <text/plugins.h>
#include <text/warnings.h>

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
  void DescribeTXTPlugin(PluginInformation& info)
  {
    info.Id = TXT_PLUGIN_ID;
    info.Description = Text::TXT_PLUGIN_INFO;
    info.Version = TXT_PLUGIN_VERSION;
    info.Capabilities = CAP_STOR_MODULE | CAP_DEV_AYM | CAP_CONV_RAW |
      GetSupportedAYMFormatConvertors() | GetSupportedVortexFormatConvertors();
  }

  class TXTHolder : public Holder, public boost::enable_shared_from_this<TXTHolder>
  {
  public:
    //region must be filled
    TXTHolder(const MetaContainer& container, const ModuleRegion& region)
    {
      const char* const dataIt = static_cast<const char*>(container.Data->Data());

      ThrowIfError(Vortex::ConvertFromText(std::string(dataIt, dataIt + region.Size),
        Data, Version, FreqTableName));
      //meta properties
      ExtractMetaProperties(TXT_PLUGIN_ID, container, region, region, Data.Info.Properties, RawData);
      //fix samples and ornaments
      std::for_each(Data.Ornaments.begin(), Data.Ornaments.end(), std::mem_fun_ref(&Vortex::Track::Ornament::Fix));
      std::for_each(Data.Samples.begin(), Data.Samples.end(), std::mem_fun_ref(&Vortex::Track::Sample::Fix));

      Vortex::Track::CalculateTimings(Data, Data.Info.Statistic.Frame, Data.Info.LoopFrame);
    }

    virtual void GetPluginInformation(PluginInformation& info) const
    {
      DescribeTXTPlugin(info);
    }

    virtual void GetModuleInformation(Information& info) const
    {
      info = Data.Info;
    }

    virtual void ModifyCustomAttributes(const Parameters::Map& attrs, bool replaceExisting)
    {
      return Parameters::MergeMaps(Data.Info.Properties, attrs, Data.Info.Properties, replaceExisting);
    }

    virtual Player::Ptr CreatePlayer() const
    {
      return Vortex::CreatePlayer(shared_from_this(), Data, Version, FreqTableName, AYM::CreateChip());
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
      else if (ConvertAYMFormat(boost::bind(&Vortex::CreatePlayer, shared_from_this(), boost::cref(Data), Version, FreqTableName, _1),
        param, dst, result))
      {
        return result;
      }
      else if (ConvertVortexFormat(Data, param, Version, FreqTableName, dst, result))
      {
        return result;
      }
      return Error(THIS_LINE, ERROR_MODULE_CONVERT, Text::MODULE_ERROR_CONVERSION_UNSUPPORTED);
    }
  private:
    Dump RawData;
    Vortex::Track::ModuleData Data;
    uint_t Version;
    String FreqTableName;
  };

  //////////////////////////////////////////////////////////////////////////
  inline bool CheckSymbol(char sym)
  {
    return !(sym >= ' ' || sym == '\r' || sym == '\n');
  }

  bool CreateTXTModule(const Parameters::Map& /*commonParams*/, const MetaContainer& container,
    Holder::Ptr& holder, ModuleRegion& region)
  {
    const std::size_t dataSize = container.Data->Size();
    const char* const data = static_cast<const char*>(container.Data->Data());
    if (dataSize < sizeof(TXT_MODULE_ID) ||
        0 != std::memcmp(data, TXT_MODULE_ID, sizeof(TXT_MODULE_ID)))
    {
      return false;
    }
    
    const char* const dataEnd = std::find_if(data, data + std::min(MAX_MODULE_SIZE, dataSize), CheckSymbol);
    const std::size_t limit = dataEnd - data;

    if (limit < MIN_MODULE_SIZE)
    {
      return false;
    }

    ModuleRegion tmpRegion;
    tmpRegion.Size = limit;
    //try to create holder
    try
    {
      holder.reset(new TXTHolder(container, tmpRegion));
  #ifdef SELF_TEST
      holder->CreatePlayer();
  #endif
      region = tmpRegion;
      return true;
    }
    catch (const Error& e)
    {
      Log::Debug("TXTSupp", "Failed to create holder ('%1%')", e.GetText());
    }
    return false;
  }
}

namespace ZXTune
{
  void RegisterTXTSupport(PluginsEnumerator& enumerator)
  {
    PluginInformation info;
    DescribeTXTPlugin(info);
    enumerator.RegisterPlayerPlugin(info, CreateTXTModule);
  }
}
