/**
 *
 * @file
 *
 * @brief  KSS format support tools implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "core/plugins/players/gme/kss_supp.h"

#include "module/players/platforms.h"

#include "contract.h"
#include "string_view.h"

namespace Module::KSS
{
  StringView DetectPlatform(Binary::View data)
  {
    Require(data.Size() >= 16);
    // do not check signatures or other
    const auto deviceFlag = data.As<uint8_t>()[0x0f];
    const bool sn76489 = deviceFlag & 2;
    if (sn76489)
    {
      // sega-based
      const bool gameGearStereo = deviceFlag & 4;
      const bool fmUnit = deviceFlag & 1;
      if (gameGearStereo)
      {
        return Platforms::GAME_GEAR;
      }
      else if (fmUnit)
      {
        return Platforms::SEGA_MASTER_SYSTEM;
      }
      else
      {
        return Platforms::SG_1000;
      }
    }
    else
    {
      // msx-based
      /*
      msxMusic is usually embedded to msx2 machines, but may be shipped as a cartridge for msx1 machines afaik
      majutsushi/scc is just a cartridge extension from konami

      so, just mark all of these platforms as MSX
      */

      /*
      enum
      {
        NONE = 0,
        MSX_AUDIO = 1,
        MAJUTSUSHI = 2,
        MSX_STEREO = 3,
      };
      const bool msxMusic = deviceFlag & 1;
      const auto extDevices = ((deviceFlag >> 3) & 3);
      const bool msxRam = deviceFlag & 4;
      const bool extRam = deviceFlag & 128;
      const bool scc = !msxRam && !extRam;
      */
      return Platforms::MSX;
    }
  }
}  // namespace Module::KSS
