/*
Abstract:
  Parameters::ZXTune::Core::Plugins implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

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
      namespace Plugins
      {
        extern const NameType PREFIX = Core::PREFIX + "plugins";

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

        namespace AY
        {
          extern const NameType PREFIX = Plugins::PREFIX + "ay";

          extern const NameType DEFAULT_DURATION_FRAMES = PREFIX + "default_duration";
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
