/**
 *
 * @file
 *
 * @brief  DirectSound subsystem access functions
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

// common includes
#include <types.h>
// library includes
#include <l10n/markup.h>
#include <sound/backend_attrs.h>
#include <tools/iterators.h>

namespace Sound::DirectSound
{
  constexpr const auto BACKEND_ID = "dsound"_id;
  constexpr auto BACKEND_DESCRIPTION = L10n::translate("DirectSound support backend.");

  class Device
  {
  public:
    using Ptr = std::shared_ptr<const Device>;
    using Iterator = ObjectIterator<Ptr>;
    virtual ~Device() = default;

    virtual String Id() const = 0;
    virtual String Name() const = 0;
  };

  Device::Iterator::Ptr EnumerateDevices();
}  // namespace Sound::DirectSound
