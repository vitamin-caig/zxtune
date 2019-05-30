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
#include "io/impl/l10n.h"

namespace Module
{
  const L10n::TranslateFunctor translate = L10n::TranslateFunctor("core");
}
