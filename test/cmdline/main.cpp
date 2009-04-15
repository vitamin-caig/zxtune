#include <sound.h>
#include <player.h>
#include <sound_attrs.h>

#include <error.h>

#include <iostream>

namespace
{
  std::ostream& operator << (std::ostream& str, const StringMap& sm)
  {
    for (StringMap::const_iterator it = sm.begin(), lim = sm.end(); it != lim; ++it)
    {
      str << it->first << ": " << it->second << '\n';
    }
    return str;
  }

  std::ostream& operator << (std::ostream& str, const ZXTune::Player::Info& info)
  {
    return str << "Type: " << info.Type << "\n" << info.Properties;
  }
}

int main(int argc, char* argv[])
{
  try
  {
    ZXTune::Sound::Parameters sndParams;
    sndParams.Frequency = 44100;
    sndParams.ClockFreq = 1750000;
    sndParams.Flags = ZXTune::Sound::PSG_TYPE_AY;

    String filename;
    for (int arg = 1; arg != argc; ++arg)
    {
      std::string args(argv[arg]);
      if (args == "--help")
      {
        std::vector<ZXTune::Player::Info> infos;
        ZXTune::GetSupportedPlayers(infos);
        std::cout << "Supported module types:\n";
        for (std::vector<ZXTune::Player::Info>::const_iterator it = infos.begin(), lim = infos.end(); it != lim; ++it)
        {
          std::cout << "Type: " << it->Type << '\n' << it->Properties << "\n------\n";
        }
        return 0;
      }

      if (arg == argc - 1)
      {
        filename = args;
      }
    }
    if (filename.empty())
    {
      std::cout << "Invalid file name specified" << std::endl;
      return 1;
    }
    return 0;
  }
  catch (const Error& e)
  {
    return static_cast<int>(e.Code);
  }
}
