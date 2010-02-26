/*
Abstract:
  Conversion helpers implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#include "convert_helpers.h"

#include <core/convert_parameters.h>
#include <core/error_codes.h>
#include <core/plugin_attrs.h>
#include <sound/dummy_receiver.h>
#include <sound/render_params.h>

#include <text/core.h>

#define FILE_TAG ED36600C

namespace ZXTune
{
  namespace Module
  {
    bool ConvertAYMFormat(const boost::function<Player::Ptr(AYM::Chip::Ptr)>& creator, const Conversion::Parameter& param, Dump& dst, Error& result)
    {
      using namespace Conversion;
      Dump tmp;
      AYM::Chip::Ptr player;
      String errMsg;

      if (parameter_cast<PSGConvertParam>(&param))
      {
        player = AYM::CreatePSGDumper(tmp);
        errMsg = TEXT_MODULE_ERROR_CONVERT_PSG;
      }
      else if (parameter_cast<ZX50ConvertParam>(&param))
      {
        player = AYM::CreateZX50Dumper(tmp);
        errMsg = TEXT_MODULE_ERROR_CONVERT_ZX50;
      }

      if (player.get())
      {
        Player::Ptr player(creator(player));
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
  }
}
