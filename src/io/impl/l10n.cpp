/**
 *
 * @file
 *
 * @brief  Per-library l10n functor implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "io/impl/l10n.h"

#include "string_view.h"

namespace IO
{
  const L10n::TranslateFunctor translate = L10n::TranslateFunctor("io");
}
