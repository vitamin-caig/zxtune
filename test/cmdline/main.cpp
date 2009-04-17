#include <sound.h>
#include <player.h>
#include <sound_attrs.h>

#include <../../lib/sound/mixer.h>
#include <../../lib/sound/renderer.h>

#include <tools.h>
#include <error.h>

#include <fstream>
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
    return str << "Capabilities: 0x" << std::hex << info.Capabilities << "\n" << info.Properties;
  }

  void Writer(const void* data, std::size_t size, void* str)
  {
    std::ostream* stream(static_cast<std::ostream*>(str));
    stream->write(static_cast<const char*>(data), static_cast<std::streamsize>(size));
  }
}

int main(int argc, char* argv[])
{
  try
  {
    ZXTune::Sound::Parameters sndParams;
    sndParams.SoundFreq = 44100;
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
          std::cout << "Capabilities: 0x" << std::hex << it->Capabilities << '\n' << it->Properties << "\n------\n";
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
    Dump data;
    {
      std::ifstream file(filename.c_str(), std::ios::binary);
      file.seekg(0, std::ios::end);
      data.resize(file.tellg());
      file.seekg(0);
      file.read(safe_ptr_cast<char*>(&data[0]), static_cast<std::streamsize>(data.size()));
    }
    ZXTune::Player::Ptr player(ZXTune::Player::Create(filename, data));
    if (!player.get())
    {
      std::cout << "Unknown format" << std::endl;
      return 1;
    }
/*
    while (ZXTune::Player::MODULE_STOPPED != player->RenderFrame(sndParams, mixer.get()));
    */
    return 0;
  }
  catch (const Error& e)
  {
    return static_cast<int>(e.Code);
  }
}
