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
#include <core/plugins/registrator.h>
#include <core/plugins/players/module_properties.h>
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
    TXTHolder(PlayerPlugin::Ptr plugin, Parameters::Accessor::Ptr parameters, DataLocation::Ptr location, const ModuleRegion& region)
      : Data(Vortex::Track::ModuleData::Create())
      , Properties(ModuleProperties::Create(plugin, location))
      , Info(CreateTrackInfo(Data, AYM::LOGICAL_CHANNELS, parameters, Properties))
    {
      const IO::DataContainer::Ptr rawData = location->GetData();
      const char* const dataIt = static_cast<const char*>(rawData->Data());
      const char* const endIt = dataIt + region.Size;

      ThrowIfError(Vortex::ConvertFromText(std::string(dataIt, endIt),
        *Data, *Properties, Version, FreqTableName));

      //meta properties
      //TODO: calculate fixed data in ConvertFromText
      Properties->SetSource(region, region);
    }

    virtual Plugin::Ptr GetPlugin() const
    {
      return Properties->GetPlugin();
    }

    virtual Information::Ptr GetModuleInformation() const
    {
      return Info;
    }

    virtual Player::Ptr CreatePlayer() const
    {
      return Vortex::CreatePlayer(Info, Data, Version, FreqTableName, AYM::CreateChip());
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
      else if (ConvertAYMFormat(boost::bind(&Vortex::CreatePlayer, boost::cref(Info), boost::cref(Data), Version, FreqTableName, _1),
        param, dst, result))
      {
        return result;
      }
      else if (ConvertVortexFormat(*Data, *Info, param, Version, FreqTableName, dst, result))
      {
        return result;
      }
      return Error(THIS_LINE, ERROR_MODULE_CONVERT, Text::MODULE_ERROR_CONVERSION_UNSUPPORTED);
    }
  private:
    const Vortex::Track::ModuleData::RWPtr Data;
    const ModuleProperties::Ptr Properties;
    const Information::Ptr Info;
    uint_t Version;
    String FreqTableName;
  };

  //////////////////////////////////////////////////////////////////////////
  inline bool CheckSymbol(char sym)
  {
    return !(sym >= ' ' || sym == '\r' || sym == '\n');
  }
  
  bool CheckTXT(const IO::DataContainer& inputData)
  {
    const std::size_t dataSize = inputData.Size();
    const char* const data = static_cast<const char*>(inputData.Data());
    if (dataSize < sizeof(TXT_MODULE_ID) ||
        0 != std::memcmp(data, TXT_MODULE_ID, sizeof(TXT_MODULE_ID)))
    {
      return false;
    }
    return true;
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

    virtual bool Check(const IO::DataContainer& inputData) const
    {
      return CheckTXT(inputData);
    }
    
    virtual Module::Holder::Ptr CreateModule(Parameters::Accessor::Ptr parameters,
                                             DataLocation::Ptr location,
                                             std::size_t& usedSize) const
    {
      const IO::DataContainer::Ptr data = location->GetData();
      const std::size_t dataSize = data->Size();
      const char* const rawData = static_cast<const char*>(data->Data());
      assert(CheckTXT(*data));
      
      const char* const dataEnd = std::find_if(rawData, rawData + std::min(MAX_MODULE_SIZE, dataSize), &CheckSymbol);
      const std::size_t limit = dataEnd - rawData;

      if (limit < MIN_MODULE_SIZE)
      {
        return Module::Holder::Ptr();
      }

      ModuleRegion tmpRegion;
      tmpRegion.Size = limit;
      //try to create holder
      try
      {
        const PlayerPlugin::Ptr plugin = shared_from_this();
        const Module::Holder::Ptr holder(new TXTHolder(plugin, parameters, location, tmpRegion));
    #ifdef SELF_TEST
        holder->CreatePlayer();
    #endif
        usedSize = tmpRegion.Offset + tmpRegion.Size;
        return holder;
      }
      catch (const Error& e)
      {
        Log::Debug("Core::TXTSupp", "Failed to create holder ('%1%')", e.GetText());
      }
      return Module::Holder::Ptr();
    }
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
