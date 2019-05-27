/**
*
* @file
*
* @brief  Per-library l10n functor implementation
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "sound/backends/l10n.h"

namespace Sound
{
  const L10n::TranslateFunctor translate = L10n::TranslateFunctor("sound_backends");
}
