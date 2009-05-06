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

#include <boost/format.hpp>

#include <fstream>
#include <iomanip>
#include <iostream>

#ifdef _WIN32
#include <conio.h>
#include <Windows.h>
#endif

using namespace ZXTune;

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

  std::ostream& operator << (std::ostream& str, const ModulePlayer::Info& info)
  {
    return str << "Capabilities: 0x" << std::hex << info.Capabilities << "\n" << info.Properties;
  }

  void Writer(const void* data, std::size_t size, void* str)
  {
    std::ostream* stream(static_cast<std::ostream*>(str));
    stream->write(static_cast<const char*>(data), static_cast<std::streamsize>(size));
  }

#ifdef _WIN32
  int GetKey()
  {
    return _kbhit() ? _getch() : 0;
  }

  COORD cpos;
  void BeginState()
  {
    CONSOLE_SCREEN_BUFFER_INFO info;
    HANDLE hdl(GetStdHandle(STD_OUTPUT_HANDLE));
    GetConsoleScreenBufferInfo(hdl, &info);
    cpos = info.dwCursorPosition;
  }

  void EndState()
  {
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), cpos);
  }
#else
#endif
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
        std::vector<ModulePlayer::Info> infos;
        GetSupportedPlayers(infos);
        std::cout << "Supported module types:\n";
        for (std::vector<ModulePlayer::Info>::const_iterator it = infos.begin(), lim = infos.end(); it != lim; ++it)
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
      if (!file)
      {
        return 1;//TODO
      }
      file.seekg(0, std::ios::end);
      data.resize(file.tellg());
      file.seekg(0);
      file.read(safe_ptr_cast<char*>(&data[0]), static_cast<std::streamsize>(data.size()));
    }

//#define WAVE

#ifdef WAVE
    Sound::Backend::Ptr backend(Sound::CreateFileBackend());
#else
#ifdef _WIN32
    Sound::Backend::Ptr backend(Sound::CreateWinAPIBackend());
#else
    Sound::Backend::Ptr backend(Sound::CreateOSSBackend());
#endif
#endif

    backend->OpenModule(filename, data);

    Module::Information module;
    backend->GetModuleInfo(module);
    Sound::Backend::Parameters params;
    backend->GetSoundParameters(params);

    params.SoundParameters.Flags = Sound::PSG_TYPE_YM | Sound::MOD_LOOP;
    params.SoundParameters.ClockFreq = 1750000;
    params.SoundParameters.SoundFreq = 48000;
    params.SoundParameters.FrameDuration = 20;
    params.BufferInMs = 20;

    if (3 == module.Statistic.Channels)
    {
      params.Mixer.resize(3);

      params.Mixer[0].Matrix[0] = Sound::FIXED_POINT_PRECISION;
      params.Mixer[0].Matrix[1] = 5 * Sound::FIXED_POINT_PRECISION / 100;
      params.Mixer[1].Matrix[0] = 66 * Sound::FIXED_POINT_PRECISION / 100;
      params.Mixer[1].Matrix[1] = 66 * Sound::FIXED_POINT_PRECISION / 100;
      params.Mixer[2].Matrix[0] = 5 * Sound::FIXED_POINT_PRECISION / 100;
      params.Mixer[2].Matrix[1] = Sound::FIXED_POINT_PRECISION;
    }
    else if (4 == module.Statistic.Channels)
    {
      params.Mixer.resize(4);

      params.Mixer[0].Matrix[0] = Sound::FIXED_POINT_PRECISION;
      params.Mixer[0].Matrix[1] = 5 * Sound::FIXED_POINT_PRECISION / 100;
      params.Mixer[1].Matrix[0] = Sound::FIXED_POINT_PRECISION;
      params.Mixer[1].Matrix[1] = 5 * Sound::FIXED_POINT_PRECISION / 100;
      params.Mixer[2].Matrix[0] = 5 * Sound::FIXED_POINT_PRECISION / 100;
      params.Mixer[2].Matrix[1] = Sound::FIXED_POINT_PRECISION;
      params.Mixer[3].Matrix[0] = 5 * Sound::FIXED_POINT_PRECISION / 100;
      params.Mixer[3].Matrix[1] = Sound::FIXED_POINT_PRECISION;
    }
#ifdef WAVE
    params.DriverParameters = "test.wav";
#else
#ifdef _WIN32
    params.DriverParameters = "2";
    params.DriverFlags = 3;
#else
#endif
#endif

    backend->SetSoundParameters(params);

    backend->Play();

    std::cout << "Module: \n" << module.Properties;

    boost::format formatter(
      "Position: %1$2d / %2% (%3%)\n"
      "Pattern:  %4$2d / %5%\n"
      "Line:     %6$2d / %7%\n"
      "Channels: %8$2d / %9%\n"
      "Tempo:    %10$2d / %11%\n"
      "Frame: %12$5d / %13%");
    for (;;)
    {
      uint32_t frame;
      Module::Tracking track;
      backend->GetModuleState(frame, track);
      BeginState();
      std::cout <<
      formatter % track.Position % module.Statistic.Position % module.Loop %
                  track.Pattern % module.Statistic.Pattern %
                  track.Note % module.Statistic.Note %
                  track.Channels % module.Statistic.Channels %
                  track.Tempo % module.Statistic.Tempo %
                  frame % module.Statistic.Frame;
      EndState();
      const int key(GetKey());
      if ('q' == key || 'Q' == key)
      {
        break;
      }
      switch (key)
      {
      case 'p':
        Sound::Backend::PLAYING == backend->GetState() ? backend->Pause() : backend->Play();
        break;
      case 's':
        backend->Stop();
        break;
      }
      IPC::Sleep(20);
    }
    //backend->Stop();
    return 0;
  }
  catch (const Error& e)
  {
    return static_cast<int>(e.Code);
  }
}
