/**
 *
 * @file
 *
 * @brief  Per-library l10n functor implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "core/plugins/archives/l10n.h"

#include "string_view.h"

namespace ZXTune
{
  const L10n::TranslateFunctor translate = L10n::TranslateFunctor("core_archives");
}
