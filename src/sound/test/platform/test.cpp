/**
 *
 * @file
 *
 * @brief  Platform test
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include <sound/backends/alsa.h>
#include <sound/backends/dsound.h>
#include <sound/backends/openal.h>
#include <sound/backends/win32.h>

#include <strings/format.h>

#include <iostream>

namespace
{
  void ShowAlsaDevicesAndMixers()
  {
    using namespace Sound::Alsa;
    std::cout << "ALSA devices and mixers:" << std::endl;
    for (const Device::Iterator::Ptr devices = EnumerateDevices(); devices->IsValid(); devices->Next())
    {
      const Device::Ptr device = devices->Get();
      std::cout << Strings::Format("Name: '{}' Card: '{}' Id: '{}'\n", device->Name(), device->CardName(),
                                   device->Id());
      const Strings::Array& mixers = device->Mixers();
      for (const auto& mixer : mixers)
      {
        std::cout << ' ' << mixer << std::endl;
      }
    }
  }

  void ShowDirectSoundDevices()
  {
    using namespace Sound::DirectSound;
    std::cout << "DirectSound devices:" << std::endl;
    for (const Device::Iterator::Ptr devices = EnumerateDevices(); devices->IsValid(); devices->Next())
    {
      const Device::Ptr device = devices->Get();
      std::cout << Strings::Format(" Name: '{}' Id: '{}'", device->Name(), device->Id()) << std::endl;
    }
  }

  void ShowOpenAlDevices()
  {
    using namespace Sound::OpenAl;
    std::cout << "OpenAL devices:" << std::endl;
    const Strings::Array& devices = EnumerateDevices();
    for (const auto& device : devices)
    {
      std::cout << Strings::Format(" Name: {}", device) << std::endl;
    }
  }

  void ShowWin32Devices()
  {
    using namespace Sound::Win32;
    std::cout << "Win32 devices:" << std::endl;
    for (const Device::Iterator::Ptr devices = EnumerateDevices(); devices->IsValid(); devices->Next())
    {
      const Device::Ptr device = devices->Get();
      std::cout << Strings::Format(" Name: '{}' Id: '{}'", device->Name(), device->Id()) << std::endl;
    }
  }
}  // namespace

int main()
{
  ShowAlsaDevicesAndMixers();
  ShowDirectSoundDevices();
  ShowOpenAlDevices();
  ShowWin32Devices();
}
