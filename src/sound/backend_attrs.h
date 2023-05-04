/**
 *
 * @file
 *
 * @brief  Backends attributes
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

// common includes
#include <types.h>

namespace Sound
{
  //! @brief Set of backend capabilities
  enum
  {
    //! Type-related capabilities
    CAP_TYPE_MASK = 0xff,
    //! Stub purposes
    CAP_TYPE_STUB = 1,
    //! Real sound subsystem playback
    CAP_TYPE_SYSTEM = 2,
    //! Saving to file capabilities
    CAP_TYPE_FILE = 4,
    //! Specific devices playback
    CAP_TYPE_HARDWARE = 8,

    //! Features-related capabilities
    CAP_FEAT_MASK = 0xff00,
    //! Hardware volume control
    CAP_FEAT_HWVOLUME = 0x100
  };

  class BackendId : public StringView
  {
    constexpr BackendId(const char* str, std::size_t size)
      : StringView(str, size)
    {}

  public:
    friend constexpr BackendId operator""_id(const char*, std::size_t) noexcept;

    static BackendId FromString(StringView s)
    {
      return BackendId(s.data(), s.size());
    }
  };

  constexpr BackendId operator"" _id(const char* str, std::size_t size) noexcept
  {
    return BackendId(str, size);
  }
}  // namespace Sound
