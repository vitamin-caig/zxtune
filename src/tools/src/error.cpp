/**
 *
 * @file
 *
 * @brief  Error object implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "error.h"

#include "strings/format.h"

#include "string_type.h"
#include "string_view.h"

#include <memory>

namespace
{
  String AttributesToString(Error::Location loc, StringView text) noexcept
  {
    constexpr auto FORMAT =
        "{0}\n"
        "@{1}\n"
        "--------\n"sv;
    return Strings::Format(FORMAT, text, loc);
  }
}  // namespace

String Error::ToString() const noexcept
{
  String details;
  for (auto meta = ErrorMeta; meta; meta = meta->Suberror)
  {
    details += AttributesToString(meta->Source, meta->Text);
  }
  return details;
}
