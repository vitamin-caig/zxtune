/*
Abstract:
  Conversion helpers implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#include "convert_helpers.h"
#include "vortex_io.h"

#include <core/convert_parameters.h>
#include <core/error_codes.h>
#include <core/freq_tables.h>
#include <core/module_attrs.h>
#include <core/plugin_attrs.h>
#include <sound/dummy_receiver.h>
#include <sound/render_params.h>

#include <boost/algorithm/string.hpp>

#include <text/core.h>

#define FILE_TAG ED36600C

namespace
{
  uint_t GetVortexNotetable(const String& freqTable)
  {
    using namespace ZXTune::Module;
    if (freqTable == TABLE_SOUNDTRACKER)
    {
      return Vortex::SOUNDTRACKER;
    }
    else if (freqTable == TABLE_PROTRACKER3_3_ASM ||
             freqTable == TABLE_PROTRACKER3_4_ASM ||
             freqTable == TABLE_ASM)
    {
      return Vortex::ASM;
    }
    else if (freqTable == TABLE_PROTRACKER3_3_REAL ||
             freqTable == TABLE_PROTRACKER3_4_REAL)
    {
      return Vortex::REAL;
    }
    else
    {
      return Vortex::PROTRACKER;
    }
  }

  uint_t GetVortexVersion(const String& freqTable)
  {
    using namespace ZXTune::Module;
    return (freqTable == TABLE_PROTRACKER3_4 ||
            freqTable == TABLE_PROTRACKER3_4_ASM ||
            freqTable == TABLE_PROTRACKER3_4_REAL) ? 4 : 3;
  }
}

namespace ZXTune
{
  namespace Module
  {
    //aym-based conversion
    bool ConvertAYMFormat(const boost::function<Player::Ptr(AYM::Chip::Ptr)>& creator, const Conversion::Parameter& param, Dump& dst, Error& result)
    {
      using namespace Conversion;
      Dump tmp;
      AYM::Chip::Ptr chip;
      String errMsg;

      if (parameter_cast<PSGConvertParam>(&param))
      {
        chip = AYM::CreatePSGDumper(tmp);
        errMsg = TEXT_MODULE_ERROR_CONVERT_PSG;
      }
      else if (parameter_cast<ZX50ConvertParam>(&param))
      {
        chip = AYM::CreateZX50Dumper(tmp);
        errMsg = TEXT_MODULE_ERROR_CONVERT_ZX50;
      }

      if (chip.get())
      {
        Player::Ptr player(creator(chip));
        Sound::DummyReceiverObject<Sound::MultichannelReceiver> receiver;
        Sound::RenderParameters params;
        for (Player::PlaybackState state = Player::MODULE_PLAYING; Player::MODULE_PLAYING == state;)
        {
          if (const Error& err = player->RenderFrame(params, state, receiver))
          {
            result = Error(THIS_LINE, ERROR_MODULE_CONVERT, errMsg).AddSuberror(err);
            return true;
          }
        }
        dst.swap(tmp);
        result = Error();
        return true;
      }
      return false;
    }

    uint_t GetSupportedAYMFormatConvertors()
    {
      return CAP_CONV_PSG | CAP_CONV_ZX50;
    }

    //vortex-based conversion
    bool ConvertVortexFormat(const Vortex::Track::ModuleData& data, const Conversion::Parameter& param,
      uint_t defaultVersion, const String& defaultFreqTable,
      Dump& dst, Error& result)
    {
      using namespace Conversion;

      if (const TXTConvertParam* txtParam = parameter_cast<TXTConvertParam>(&param))
      {
        Vortex::Description descr;
        const Parameters::Map::const_iterator titleIt(data.Info.Properties.find(Module::ATTR_TITLE));
        if (titleIt != data.Info.Properties.end())
        {
          descr.Title = Parameters::ConvertToString(titleIt->second);
        }
        const Parameters::Map::const_iterator authorIt(data.Info.Properties.find(Module::ATTR_AUTHOR));
        if (authorIt != data.Info.Properties.end())
        {
          descr.Author = Parameters::ConvertToString(authorIt->second);
        }
        const uint_t version = in_range<uint_t>(txtParam->Version, 1, 9) ? txtParam->Version : defaultVersion;
        const String freqTable(txtParam->FreqTable.empty() ? defaultFreqTable : txtParam->FreqTable);
        descr.Version = 30 + (in_range<uint_t>(version, 1, 9) ? version : GetVortexVersion(freqTable));
        descr.Notetable = GetVortexNotetable(freqTable);
        descr.Tempo = data.Info.Statistic.Tempo;
        descr.Loop = data.Info.LoopPosition;
        descr.Order = data.Positions;

        typedef std::vector<std::string> LinesArray;
        LinesArray asArray;
        std::back_insert_iterator<LinesArray> iter(asArray);
        *iter = Vortex::DescriptionHeaderToString();
        iter = Vortex::DescriptionToStrings(descr, ++iter);
        *iter = std::string();//free
        //store ornaments
        for (uint_t idx = 1; idx != data.Ornaments.size(); ++idx)
        {
          if (data.Ornaments[idx].Data.size())
          {
            *++iter = Vortex::OrnamentHeaderToString(idx);
            *++iter = Vortex::OrnamentToString(data.Ornaments[idx]);
            *++iter = std::string();//free
          }
        }
        //store samples
        for (uint_t idx = 1; idx != data.Samples.size(); ++idx)
        {
          if (data.Samples[idx].Data.size())
          {
            *++iter = Vortex::SampleHeaderToString(idx);
            iter = Vortex::SampleToStrings(data.Samples[idx], ++iter);
            *++iter = std::string();
          }
        }
        //store patterns
        for (uint_t idx = 0; idx != data.Patterns.size(); ++idx)
        {
          if (data.Patterns[idx].size())
          {
            *++iter = Vortex::PatternHeaderToString(idx);
            iter = Vortex::PatternToStrings(data.Patterns[idx], ++iter);
            *++iter = std::string();
          }
        }
        const char DELIMITER[] = {'\n', 0};
        const std::string& asString(boost::algorithm::join(asArray, DELIMITER));
        dst.assign(asString.begin(), asString.end());
        result = Error();
        return true;
      }
      return false;
    }

    uint_t GetSupportedVortexFormatConvertors()
    {
      return CAP_CONV_TXT;
    }
  }
}
