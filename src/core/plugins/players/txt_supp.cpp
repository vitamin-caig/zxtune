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
  const String TEXT_TXT_VERSION(FromStdString("$Rev$"));

  const std::size_t MIN_MODULE_SIZE = 256;
  const std::size_t MAX_MODULE_SIZE = 1048576;//1M is more than enough

  inline bool FindSection(const String& str)
  {
    return str.size() >= 3 && str[0] == '[' && str[str.size() - 1] == ']';
  }

  ////////////////////////////////////////////
  void DescribeTXTPlugin(PluginInformation& info)
  {
    info.Id = TXT_PLUGIN_ID;
    info.Description = TEXT_TXT_INFO;
    info.Version = TEXT_TXT_VERSION;
    info.Capabilities = CAP_STOR_MODULE | CAP_DEV_AYM | CAP_CONV_RAW | GetSupportedAYMFormatConvertors();
  }

  class TXTHolder : public Holder, public boost::enable_shared_from_this<TXTHolder>
  {
  public:
    //region must be filled
    TXTHolder(const MetaContainer& container, const ModuleRegion& region)
    {
      const IO::FastDump& data = IO::FastDump(*container.Data);
      const char* const dataIt(safe_ptr_cast<const char*>(data.Data()));
      const boost::iterator_range<const char*> range(dataIt, dataIt + region.Size);

      typedef std::vector<std::string> LinesArray;
      LinesArray lines;
      boost::algorithm::split(lines, range, boost::algorithm::is_cntrl(),
        boost::algorithm::token_compress_on);

      Vortex::Description descr;

      Formatter fmt(TEXT_TXT_ERROR_INVALID_STRING);
      for (LinesArray::const_iterator it = lines.begin(), lim = lines.end(); it != lim;)
      {
        const std::string& string(*it);
        const LinesArray::const_iterator next(std::find_if(++it, lim, FindSection));
        uint_t idx(0);
        if (Vortex::DescriptionHeaderFromString(string))
        {
          const LinesArray::const_iterator stop(Vortex::DescriptionFromStrings(it, next, descr));
          if (next != stop)
          {
            throw Error(THIS_LINE, ERROR_FIND_PLAYER_PLUGIN, (fmt % *stop).str());
          }
        }
        else if (Vortex::OrnamentHeaderFromString(string, idx))
        {
          Data.Ornaments.resize(idx + 1);
          if (!Vortex::OrnamentFromString(*it, Data.Ornaments[idx]))
          {
            throw Error(THIS_LINE, ERROR_FIND_PLAYER_PLUGIN, (fmt % *it).str());
          }
        }
        else if (Vortex::SampleHeaderFromString(string, idx))
        {
          Data.Samples.resize(idx + 1);
          const StringArray::const_iterator stop(SampleFromStrings(it, next, Data.Samples[idx]));
          if (next != stop)
          {
            throw Error(THIS_LINE, ERROR_FIND_PLAYER_PLUGIN, (fmt % *stop).str());
          }
        }
        else if (Vortex::PatternHeaderFromString(string, idx))
        {
          Data.Patterns.resize(idx + 1);
          const LinesArray::const_iterator stop(Vortex::PatternFromStrings(it, next, Data.Patterns[idx]));
          if (next != stop)
          {
            throw Error(THIS_LINE, ERROR_FIND_PLAYER_PLUGIN, (fmt % *stop).str());
          }
        }
        else
        {
          throw Error(THIS_LINE, ERROR_FIND_PLAYER_PLUGIN, (fmt % string).str());
        }
        it = next;
      }

      //fix samples and ornaments
      std::for_each(Data.Ornaments.begin(), Data.Ornaments.end(), std::mem_fun_ref(&Vortex::Track::Ornament::Fix));
      std::for_each(Data.Samples.begin(), Data.Samples.end(), std::mem_fun_ref(&Vortex::Track::Sample::Fix));

      //meta properties
      ExtractMetaProperties(TXT_PLUGIN_ID, container, region, region, Data.Info.Properties, RawData);
      if (!descr.Title.empty())
      {
        Data.Info.Properties.insert(Parameters::Map::value_type(ATTR_TITLE, descr.Title));
      }
      if (!descr.Author.empty())
      {
        Data.Info.Properties.insert(Parameters::Map::value_type(ATTR_AUTHOR, descr.Author));
      }
      Data.Info.Properties.insert(Parameters::Map::value_type(Module::ATTR_PROGRAM,
        (Formatter(TEXT_VORTEX_EDITOR) % (descr.Version / 10) % (descr.Version % 10)).str()));

      //tracking properties
      Version = descr.Version % 10;
      switch (descr.Notetable)
      {
      case 0://PT
        FreqTableName = Version <= 3 ? TABLE_PROTRACKER3_3 : TABLE_PROTRACKER3_4;
        break;
      case 1://ST
        FreqTableName = TABLE_SOUNDTRACKER;
        break;
      case 2://ASM
        FreqTableName = Version <= 3 ? TABLE_PROTRACKER3_3_ASM : TABLE_PROTRACKER3_4_ASM;
        break;
      default:
        FreqTableName = Version <= 3 ? TABLE_PROTRACKER3_3_REAL : TABLE_PROTRACKER3_4_REAL;
      }
      Data.Info.LoopPosition = descr.Loop;
      Data.Info.PhysicalChannels = AYM::CHANNELS;
      Data.Info.Statistic.Tempo = descr.Tempo;
      Data.Info.Statistic.Position = descr.Order.size();
      Data.Positions.swap(descr.Order);
      Data.Info.Statistic.Pattern = std::count_if(Data.Patterns.begin(), Data.Patterns.end(),
        !boost::bind(&Vortex::Track::Pattern::empty, _1));
      Data.Info.Statistic.Channels = AYM::CHANNELS;

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
      }
      else if (!ConvertAYMFormat(boost::bind(&Vortex::CreatePlayer, shared_from_this(), boost::cref(Data), Version, FreqTableName, _1),
        param, dst, result))
      {
        return Error(THIS_LINE, ERROR_MODULE_CONVERT, TEXT_MODULE_ERROR_CONVERSION_UNSUPPORTED);
      }
      return result;
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
    return !(std::isprint(sym) || sym == '\r' || sym == '\n');
  }

  bool CreateTXTModule(const Parameters::Map& /*commonParams*/, const MetaContainer& container,
    Holder::Ptr& holder, ModuleRegion& region)
  {
    const char* const data(static_cast<const char*>(container.Data->Data()));
    const char* const dataEnd(std::find_if(data, data + std::min(MAX_MODULE_SIZE, container.Data->Size()), CheckSymbol));
    const std::size_t limit(dataEnd - data);

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
      Log::Debug("TXTSupp", "Failed to create holder ", e.GetText());
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
