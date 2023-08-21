/**
 *
 * @file
 *
 * @brief  SC68 replayers database interface
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// common includes
#include <string_view.h>
// library includes
#include <binary/view.h>

namespace Module::SC68
{
  Binary::View GetReplayer(StringView name);
}  // namespace Module::SC68
