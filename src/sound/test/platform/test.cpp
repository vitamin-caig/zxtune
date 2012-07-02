#include <sound/backends/alsa.h>
#include <sound/backends/dsound.h>
#include <sound/backends/win32.h>
#include <iostream>
#include <format.h>

namespace
{
  void ShowAlsaDevicesAndMixers()
  {
    using namespace ZXTune::Sound::ALSA;
    std::cout << "ALSA devices and mixers:" << std::endl;
    for (const Device::Iterator::Ptr devices = EnumerateDevices(); devices->IsValid(); devices->Next())
    {
      const Device::Ptr device = devices->Get();
      std::cout << Strings::Format("Name: '%1%' Card: '%2%' Id: '%3%'\n", device->Name(), device->CardName(), device->Id());
      const StringArray& mixers = device->Mixers();
      for (StringArray::const_iterator mit = mixers.begin(), mlim = mixers.end(); mit != mlim; ++mit)
      {
        std::cout << ' ' << *mit << std::endl;
      }
    }
  }

  void ShowDirectSoundDevices()
  {
    using namespace ZXTune::Sound::DirectSound;
    std::cout << "DirectSound devices:" << std::endl;
    for (const Device::Iterator::Ptr devices = EnumerateDevices(); devices->IsValid(); devices->Next())
    {
      const Device::Ptr device = devices->Get();
      std::cout << Strings::Format(" Name: '%1%' Id: '%2%'", device->Name(), device->Id()) << std::endl;
    }
  }

  void ShowWin32Devices()
  {
    using namespace ZXTune::Sound::Win32;
    std::cout << "Win32 devices:" << std::endl;
    for (const Device::Iterator::Ptr devices = EnumerateDevices(); devices->IsValid(); devices->Next())
    {
      const Device::Ptr device = devices->Get();
      std::cout << Strings::Format(" Name: '%1%' Id: '%2%'", device->Name(), device->Id()) << std::endl;
    }
  }
}

int main()
{
  ShowAlsaDevicesAndMixers();
  ShowDirectSoundDevices();
  ShowWin32Devices();
}
