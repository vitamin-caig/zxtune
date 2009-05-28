#include <sound.h>
#include <player.h>
#include <sound_attrs.h>

#include <../../lib/sound/mixer.h>
#include <../../lib/sound/renderer.h>

#include <../../supp/sound_backend.h>
#include <../../supp/sound_backend_types.h>

#include <tools.h>
#include <error.h>

#include <boost/format.hpp>
#include <boost/thread.hpp>

#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <conio.h>
#include <windows.h>
#else
#include <termio.h>
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

  void MoveUp(std::size_t lines)
  {
    CONSOLE_SCREEN_BUFFER_INFO info;
    HANDLE hdl(GetStdHandle(STD_OUTPUT_HANDLE));
    GetConsoleScreenBufferInfo(hdl, &info);
    info.dwCursorPosition.Y -= lines;
    info.dwCursorPosition.X = 0;
    SetConsoleCursorPosition(hdl, info.dwCursorPosition);
  }
#else
  int GetKey()
  {
    struct termios oldt, newt;
    int ch;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    newt.c_cc[VMIN] = 0;
    newt.c_cc[VTIME] = 1;
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    ch = getchar();
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    return ch;
  }

  void MoveUp(std::size_t lines)
  {
    std::cout << "\r\x1b[" << lines << 'A';
  }
#endif

  template<class T>
  T Decrease(T val, T speed)
  {
    return val >= speed ? val - speed : 0;
  }
}

int main(int argc, char* argv[])
{
  try
  {
    Sound::Backend::Ptr backend;
    String driverParam;
    bool silent(false);
    String filename;
    for (int arg = 1; arg != argc; ++arg)
    {
      const std::string& args(argv[arg]);
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
      else if (args == "--silent")
      {
        silent = true;
      }
      else if (args == "--null")
      {
        backend = Sound::CreateNullBackend();
      }
      else if (args == "--file")
      {
        if (arg == argc - 1)
        {
          std::cout << "Invalid output name specified" << std::endl;
          return 1;
        }
        backend = Sound::CreateFileBackend();
        driverParam = argv[++arg];
      }
#ifdef _WIN32
      else if (args == "--win32")
      {
        if (arg < argc - 2)
        {
          driverParam = argv[++arg];
        }
        backend = Sound::CreateWinAPIBackend();
      }
#else
      else if (args == "--oss")
      {
        if (arg < argc - 2)
        {
          driverParam = argv[++arg];
        }
        backend = Sound::CreateOSSBackend();
      }
      else if (args == "--alsa")
      {
        if (arg < argc - 2)
        {
          driverParam = argv[++arg];
        }
        backend = Sound::CreateAlsaBackend();
      }
#endif
      else if (arg == argc - 1)
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
      std::cout << "Reading " << filename << std::endl;
      file.seekg(0, std::ios::end);
      data.resize(file.tellg());
      file.seekg(0);
      file.read(safe_ptr_cast<char*>(&data[0]), static_cast<std::streamsize>(data.size()));
    }

//#define WAVE

    if (!backend.get())
    {
#ifdef _WIN32
      backend = Sound::CreateWinAPIBackend();
#else
      backend = Sound::CreateOSSBackend();
#endif
    }
    backend->OpenModule(filename, data);

    Module::Information module;
    backend->GetModuleInfo(module);
    Sound::Backend::Parameters params;
    backend->GetSoundParameters(params);

    params.SoundParameters.Flags = Sound::PSG_TYPE_YM | Sound::MOD_LOOP;
    params.SoundParameters.ClockFreq = 1750000;
    params.SoundParameters.SoundFreq = 48000;
    params.SoundParameters.FrameDuration = 20;
    params.BufferInMs = 100;

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
    params.DriverParameters = driverParam;
    params.DriverFlags = 2;

    backend->SetSoundParameters(params);

    backend->Play();

    if (!silent)
    {
      std::cout << "Module: \n" << module.Properties;
    }

    boost::format formatter(
      "Position: %1$2d / %2% (%3%)\n"
      "Pattern:  %4$2d / %5%\n"
      "Line:     %6$2d / %7%\n"
      "Channels: %8$2d / %9%\n"
      "Tempo:    %10$2d / %11%\n"
      "Frame: %12$5d / %13%");
    Sound::Analyze::Volume volState;
    Sound::Analyze::Spectrum specState;
    while (Sound::Backend::STOPPED != backend->GetState())
    {
      std::size_t frame;
      Module::Tracking track;
      backend->GetModuleState(frame, track);
      if (!silent)
      {
        std::cout <<
        (formatter % (track.Position + 1) % module.Statistic.Position % (1 + module.Loop) %
                    track.Pattern % module.Statistic.Pattern %
                    track.Note % module.Statistic.Note %
                    track.Channels % module.Statistic.Channels %
                    track.Tempo % module.Statistic.Tempo %
                    frame % module.Statistic.Frame);
        backend->GetSoundState(volState, specState);
        const std::size_t WIDTH = 75;
        const std::size_t HEIGTH = 16;
        const std::size_t LIMIT = std::numeric_limits<Sound::Analyze::Level>::max();
        const std::size_t FALLSPEED = 8;
        static char filler[WIDTH + 1];
        const std::size_t specShift(4/*volState.Array.size()*/ + 1);
        static std::size_t levels[Sound::Analyze::TonesCount + specShift];
        for (std::size_t tone = 0; tone != ArraySize(levels); ++tone)
        {
          const std::size_t value(tone >= specShift ?
            specState.Array[tone - specShift]
            :
            (tone < volState.Array.size() ? volState.Array[tone] : 0));
          levels[tone] = value * HEIGTH / LIMIT;
        }
        for (std::size_t y = HEIGTH; y; --y)
        {
          for (std::size_t i = 0; i < std::min(WIDTH, ArraySize(levels)); ++i)
          {
            filler[i] = levels[i] > y ? '#' : ' ';
            filler[i + 1] = 0;
          }
          std::cout << std::endl << filler;
        }
        std::transform(volState.Array.begin(), volState.Array.end(), volState.Array.begin(),
          std::bind2nd(std::ptr_fun(Decrease<Sound::Analyze::Level>), FALLSPEED));
        std::transform(specState.Array.begin(), specState.Array.end(), specState.Array.begin(), 
          std::bind2nd(std::ptr_fun(Decrease<Sound::Analyze::Level>), FALLSPEED));
        MoveUp(5 + HEIGTH);
      }
      boost::this_thread::sleep(boost::posix_time::milliseconds(20));
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
      case 'z':
        if (params.Preamp)
        {
          --params.Preamp;
          backend->SetSoundParameters(params);
        }
        break;
      case 'x':
        if (params.Preamp < Sound::FIXED_POINT_PRECISION)
        {
          ++params.Preamp;
          backend->SetSoundParameters(params);
        }
        break;
      }
    }
    backend->Stop();
    return 0;
  }
  catch (const Error& e)
  {
    std::cout << e.Text << std::endl;
    return static_cast<int>(e.Code);
  }
}
