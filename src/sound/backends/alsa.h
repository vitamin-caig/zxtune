/**
 *
 * @file
 *
 * @brief  ALSA subsystem access functions
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include <l10n/markup.h>
#include <sound/backend_attrs.h>
#include <strings/array.h>
#include <tools/iterators.h>

namespace Sound::Alsa
{
  constexpr const auto BACKEND_ID = "alsa"_id;
  constexpr auto BACKEND_DESCRIPTION = L10n::translate("ALSA sound system backend");

  class Device
  {
  public:
    using Ptr = std::shared_ptr<const Device>;
    using Iterator = ObjectIterator<Ptr>;
    virtual ~Device() = default;

    virtual String Id() const = 0;
    virtual String Name() const = 0;
    virtual String CardName() const = 0;

    virtual Strings::Array Mixers() const = 0;
  };

  Device::Iterator::Ptr EnumerateDevices();
}  // namespace Sound::Alsa
