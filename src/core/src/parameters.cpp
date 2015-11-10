/**
* 
* @file
*
* @brief  Parameters::ZXTune::Core and nested
*
* @author vitamin.caig@gmail.com
*
**/

//library includes
#include <zxtune.h>
#include <core/core_parameters.h>
#include <core/plugins_parameters.h>

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

        extern const NameType SAMPLES_FREQUENCY = PREFIX + "samples_frequency";
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

      namespace SAA
      {
        extern const NameType PREFIX = Core::PREFIX + "saa";

        extern const NameType CLOCKRATE = PREFIX + "clockrate";
        extern const NameType INTERPOLATION = PREFIX + "interpolation";
      }

      namespace SID
      {
        extern const NameType PREFIX = Core::PREFIX + "sid";

        extern const NameType FILTER = PREFIX + "filter";
        extern const NameType INTERPOLATION = PREFIX + "interpolation";
      }

      namespace Plugins
      {
        extern const NameType PREFIX = Core::PREFIX + "plugins";
        
        extern const NameType DEFAULT_DURATION = PREFIX + "default_duration";

        namespace Raw
        {
          extern const NameType PREFIX = Plugins::PREFIX + "raw";

          extern const NameType PLAIN_DOUBLE_ANALYSIS = PREFIX + "plain_double_analysis";
          extern const NameType MIN_SIZE = PREFIX + "min_size";
        }

        namespace Hrip
        {
          extern const NameType PREFIX = Plugins::PREFIX + "hrip";

          extern const NameType IGNORE_CORRUPTED = PREFIX + "ignore_corrupted";
        }

        namespace SID
        {
          extern const NameType PREFIX = Plugins::PREFIX + "sid";

          extern const NameType KERNAL = PREFIX + "kernal";
          extern const NameType BASIC = PREFIX + "basic";
          extern const NameType CHARGEN = PREFIX + "chargen";
        }

        namespace Zip
        {
          extern const NameType PREFIX = Plugins::PREFIX + "zip";

          extern const NameType MAX_DEPACKED_FILE_SIZE_MB = PREFIX + "max_depacked_size_mb";
        }
      }
    }
  }
}
