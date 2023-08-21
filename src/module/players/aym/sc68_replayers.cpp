/**
 *
 * @file
 *
 * @brief  SC68 replayers database implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include "module/players/aym/sc68_replayers.h"
// common includes
#include <contract.h>
#include <pointers.h>
// std includes
#include <algorithm>

namespace Module::SC68
{
  struct Entry
  {
    const StringView Name;
    const StringView Content;

    bool operator<(StringView name) const
    {
      return Name < name;
    }

    Binary::View ToBinaryView() const
    {
      return {Content.data(), Content.size()};
    }
  };

  constexpr const Entry ENTRIES[] = {
#include "module/players/aym/sc68_replayers.inc"
  };

  Binary::View GetReplayer(StringView name)
  {
    const auto* const end = std::end(ENTRIES);
    const auto* const lower = std::lower_bound(ENTRIES, end, name);
    if (lower != end && lower->Name == name)
    {
      return lower->ToBinaryView();
    }
    return {nullptr, 0};
  }
}  // namespace Module::SC68
