#include <sound/backends/alsa.h>
#include <iostream>

namespace
{
  void ShowAlsaDevicesAndMixers()
  {
    std::cout << "ALSA devices and mixers:" << std::endl;
    const StringArray devices = ZXTune::Sound::ALSA::EnumerateDevices();
    for (StringArray::const_iterator it = devices.begin(), lim = devices.end(); it != lim; ++it)
    {
      std::cout << *it << std::endl;
      const StringArray& mixers = ZXTune::Sound::ALSA::EnumerateMixers(*it);
      for (StringArray::const_iterator mit = mixers.begin(), mlim = mixers.end(); mit != mlim; ++mit)
      {
        std::cout << ' ' << *mit << std::endl;
      }
    }
  }
}

int main()
{
  ShowAlsaDevicesAndMixers();
}
