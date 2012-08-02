/*
Abstract:
  Parameters::ZXTune::Core implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//library includes
#include <zxtune.h>
#include <core/core_parameters.h>

namespace Parameters
{
  namespace ZXTune
  {
    namespace Core
    {
      extern const NameType PREFIX = ZXTune::PREFIX + "core";

      namespace AYM
      {
        extern const NameType PREFIX = Core::PREFIX + "aym";

        extern const NameType CLOCKRATE = PREFIX + "clockrate";
        extern const NameType TYPE = PREFIX + "type";
        extern const NameType INTERPOLATION = PREFIX + "interpolation";
        extern const NameType TABLE = PREFIX + "table";
        extern const NameType DUTY_CYCLE = PREFIX + "duty_cycle";
        extern const NameType DUTY_CYCLE_MASK = PREFIX + "duty_cycle_mask";
        extern const NameType LAYOUT = PREFIX + "layout";
      }

      namespace DAC
      {
        extern const NameType PREFIX = Core::PREFIX + "dac";

        extern const NameType INTERPOLATION = PREFIX + "interpolation";
      }

      namespace Z80
      {
        extern const NameType PREFIX = Core::PREFIX + "z80";

        extern const NameType INT_TICKS = PREFIX + "int_ticks";
        extern const NameType CLOCKRATE = PREFIX + "clockrate";
      }

      namespace FM
      {
        extern const NameType PREFIX = Core::PREFIX + "fm";

        extern const NameType CLOCKRATE = PREFIX + "clockrate";
      }
    }
  }
}
