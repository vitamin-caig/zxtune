/*
Abstract:
  AY-based conversion helpers implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//local includes
#include "ay_conversion.h"
#include "vortex_io.h"
//library includes
#include <core/convert_parameters.h>
#include <core/error_codes.h>
#include <core/plugin_attrs.h>
#include <sound/render_params.h>
//boost includes
#include <boost/algorithm/string.hpp>
//text includes
#include <core/text/core.h>

#define FILE_TAG ED36600C

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

      //convert to PSG
      if (parameter_cast<PSGConvertParam>(&param))
      {
        chip = AYM::CreatePSGDumper(tmp);
        errMsg = Text::MODULE_ERROR_CONVERT_PSG;
      }
      //convert to ZX50
      else if (parameter_cast<ZX50ConvertParam>(&param))
      {
        chip = AYM::CreateZX50Dumper(tmp);
        errMsg = Text::MODULE_ERROR_CONVERT_ZX50;
      }

      if (chip.get())
      {
        Player::Ptr player(creator(chip));
        Sound::MultichannelReceiver::Ptr receiver = Sound::MultichannelReceiver::CreateStub();
        const Sound::RenderParameters::Ptr params = Sound::RenderParameters::Create(
          Parameters::Container::Create());
        for (Player::PlaybackState state = Player::MODULE_PLAYING; Player::MODULE_PLAYING == state;)
        {
          if (const Error& err = player->RenderFrame(*params, state, *receiver))
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
    bool ConvertVortexFormat(const Vortex::Track::ModuleData& data, const Information& info, const Conversion::Parameter& param,
      uint_t version, const String& freqTable,
      Dump& dst, Error& result)
    {
      using namespace Conversion;

      //convert to TXT
      if (parameter_cast<TXTConvertParam>(&param))
      {
        const std::string& asString = Vortex::ConvertToText(data, info, version, freqTable);
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
