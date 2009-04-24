#include <sound.h>
#include <player.h>
#include <sound_attrs.h>

#include <../../lib/sound/mixer.h>
#include <../../lib/sound/renderer.h>

#include <../../supp/ipc.h>
#include <../../supp/sound_backend.h>
#include <../../supp/sound_backend_types.h>

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

  std::ostream& operator << (std::ostream& str, const ZXTune::ModulePlayer::Info& info)
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
    String filename;
    for (int arg = 1; arg != argc; ++arg)
    {
      std::string args(argv[arg]);
      if (args == "--help")
      {
        std::vector<ZXTune::ModulePlayer::Info> infos;
        ZXTune::GetSupportedPlayers(infos);
        std::cout << "Supported module types:\n";
        for (std::vector<ZXTune::ModulePlayer::Info>::const_iterator it = infos.begin(), lim = infos.end(); it != lim; ++it)
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

#define WAVE

#ifdef WAVE
    ZXTune::Sound::Backend::Ptr backend(ZXTune::Sound::CreateFileBackend());
#else
    ZXTune::Sound::Backend::Ptr backend(ZXTune::Sound::CreateWinAPIBackend());
#endif

    backend->OpenModule(filename, data);

    ZXTune::Sound::Backend::Parameters params;
    backend->GetSoundParameters(params);

    params.SoundParameters.Flags = ZXTune::Sound::PSG_TYPE_YM;
    params.SoundParameters.ClockFreq = 1750000;
    params.SoundParameters.SoundFreq = 48000;
    params.SoundParameters.FrameDuration = 20;
    params.BufferInMs = 1000;

    params.Mixer.resize(3);
    
    params.Mixer[0].Matrix[0] = 100;
    params.Mixer[0].Matrix[1] = 5;
    params.Mixer[1].Matrix[0] = 66;
    params.Mixer[1].Matrix[1] = 66;
    params.Mixer[2].Matrix[0] = 5;
    params.Mixer[2].Matrix[1] = 100;
#ifdef WAVE
    params.DriverParameters = "test.wav";
#else
    params.DriverParameters = "2";
#endif

    backend->SetSoundParameters(params);

    backend->Play();

    while (true)
    {
      uint32_t frame;
      ZXTune::Module::Tracking track;
      backend->GetModuleState(frame, track);
      std::cout << "\rCurrent frame: " << frame << std::flush;
      char sym(0);
      std::cin >> sym;
      if ('q' == sym || 'Q' == sym)
      {
        break;
      }
      switch (sym)
      {
      case 'p':
        ZXTune::Sound::Backend::PLAYING == backend->GetState() ? backend->Pause() : backend->Play();
        break;
      case 's':
        backend->Stop();
        break;
      }
      //ZXTune::IPC::Sleep(1000);
    }
    //backend->Stop();
    return 0;
  }
  catch (const Error& e)
  {
    return static_cast<int>(e.Code);
  }
}
