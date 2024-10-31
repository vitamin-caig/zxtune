/**
 *
 * @file
 *
 * @brief  Per-library l10n functor implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "sound/backends/l10n.h"

#include "string_view.h"

namespace Sound
{
  const L10n::TranslateFunctor translate = L10n::TranslateFunctor("sound_backends");
}
