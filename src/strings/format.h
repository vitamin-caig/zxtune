/**
 *
 * @file
 *
 * @brief  Formatter type definition
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include <string_type.h>
#include <types.h>

#include <3rdparty/fmt/include/fmt/core.h>

#include <utility>

namespace Strings
{
  template<class... P>
  using FormatString = fmt::format_string<P...>;

  //! @brief Format functions
  //! @code
  //! const String& txt = Strings::Format(FORMAT_STRING, param1, param2);
  //! @endcode
  template<class... P>
  String Format(FormatString<P...> format, P&&... params) noexcept
  {
    return fmt::format(format, std::forward<P>(params)...);
  }

  template<class... P>
  String FormatRuntime(fmt::string_view format, P&&... params)
  {
    return fmt::format(fmt::runtime(format), std::forward<P>(params)...);
  }

  String FormatTime(uint_t hours, uint_t minutes, uint_t seconds, uint_t frames) noexcept;
}  // namespace Strings
