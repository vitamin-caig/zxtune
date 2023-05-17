/**
 *
 * @file
 *
 * @brief  Win32 subsystem access functions
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

// common includes
#include <iterator.h>
#include <types.h>
// library includes
#include <l10n/markup.h>
#include <sound/backend_attrs.h>

namespace Sound::Win32
{
  constexpr const auto BACKEND_ID = "win32"_id;
  constexpr auto BACKEND_DESCRIPTION = L10n::translate("Win32 sound system backend");

  class Device
  {
  public:
    using Ptr = std::shared_ptr<const Device>;
    using Iterator = ObjectIterator<Ptr>;
    virtual ~Device() = default;

    virtual int_t Id() const = 0;
    virtual String Name() const = 0;
  };

  Device::Iterator::Ptr EnumerateDevices();
}  // namespace Sound::Win32
