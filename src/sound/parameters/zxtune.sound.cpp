/*
Abstract:
  Parameters::ZXTune::Sound implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//library includes
#include <zxtune.h>
#include <sound/sound_parameters.h>

namespace Parameters
{
  namespace ZXTune
  {
    namespace Sound
    {
      extern const NameType PREFIX = ZXTune::PREFIX + "sound";

      extern const NameType FREQUENCY = PREFIX + "frequency";

      extern const NameType FRAMEDURATION = PREFIX + "frameduration";

      extern const NameType LOOPED = PREFIX + "looped";
    }
  }
}
