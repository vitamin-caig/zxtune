/**
 *
 * @file
 *
 * @brief  Error object implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// common includes
#include <error_tools.h>
// std includes
#include <utility>

namespace
{
  String AttributesToString(Error::Location loc, StringView text) noexcept
  {
    try
    {
      constexpr const Char FORMAT[] =
          "{0}\n"
          "@{1}\n"
          "--------\n";
      return Strings::Format(FORMAT, text, loc);
    }
    catch (const std::exception& e)
    {
      return e.what();
    }
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
