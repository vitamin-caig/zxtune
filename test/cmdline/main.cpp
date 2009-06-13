#include <sound.h>
#include <player.h>
#include <sound_attrs.h>
#include <player_attrs.h>

#include <../../lib/sound/mixer.h>
#include <../../lib/sound/renderer.h>
#include <../../lib/io/container.h>

#include <../../supp/sound_backend.h>
#include <../../supp/sound_backend_types.h>


#include <tools.h>
#include <error.h>

#include <boost/format.hpp>
#include <boost/thread.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>

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
    bool silent(false);
    String filename;
    Sound::Backend::Parameters parameters;

    //default parameters
    parameters.SoundParameters.ClockFreq = 1750000;
    parameters.SoundParameters.SoundFreq = 44100;
    parameters.SoundParameters.FrameDuration = 20;
    parameters.DriverFlags = 3;
    parameters.BufferInMs = 100;
    
    for (int arg = 1; arg != argc; ++arg)
    {
      const std::string& args(argv[arg]);
      if (args == "--help")
      {
        std::cout << argv[0] << " [parameters] filename\n"
          "Parameters. Playbacks:\n"
          "--null            -- use null backend. No sound\n"
          "--file filename   -- write to wav file\n"
#ifdef _WIN32
          "--win32 [devnum]  -- use win32 mapper (default)\n"
#else
          "--oss [device]    -- use OSS playback (default)\n"
          "--alsa [device]   -- use ALSA playback\n"
#endif
          "\nOther parameters:\n"
          "--silent          -- do not produce any output\n"
          "--ym              -- use YM PSG\n"
          "--loop            -- loop modules playback\n"
          "--clock value     -- set PSG clock (" << parameters.SoundParameters.ClockFreq << " default)\n"
          "--sound value     -- set sound frequency (" << parameters.SoundParameters.SoundFreq << " default)\n"
          "--fir order,a-b   -- use FIR with order and range from a to b\n"

          "\nModes:\n"
          "--help            -- this page\n"
          "--info            -- supported formats list\n";
        return 0;
      }
      else if (args == "--info")
      {
        std::vector<ModulePlayer::Info> infos;
        GetSupportedPlayers(infos);
        std::cout << "Supported module types:\n";
        for (std::vector<ModulePlayer::Info>::const_iterator it = infos.begin(), lim = infos.end(); it != lim; ++it)
        {
          std::cout << it->Properties <<
            "Capabilities: 0x" << std::hex << it->Capabilities << "\n------\n";
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
        parameters.DriverParameters = argv[++arg];
        parameters.DriverFlags = 0;
      }
#ifdef _WIN32
      else if (args == "--win32")
      {
        if (arg < argc - 2 && *argv[arg + 1] != '-')
        {
          parameters.DriverParameters = argv[++arg];
        }
        backend = Sound::CreateWinAPIBackend();
      }
#else
      else if (args == "--oss")
      {
        if (arg < argc - 2 && *argv[arg + 1] != '-')
        {
          parameters.DriverParameters = argv[++arg];
        }
        backend = Sound::CreateOSSBackend();
      }
      else if (args == "--alsa")
      {
        if (arg < argc - 2 && *argv[arg + 1] != '-')
        {
          parameters.DriverParameters = argv[++arg];
        }
        backend = Sound::CreateAlsaBackend();
      }
#endif
      else if (args == "--ym")
      {
        parameters.SoundParameters.Flags |= Sound::PSG_TYPE_YM;
      }
      else if (args == "--loop")
      {
        parameters.SoundParameters.Flags |= Sound::MOD_LOOP;
      }
      else if (args == "--clock")
      {
        if (arg == argc - 1)
        {
          std::cout << "Invalid psg clock freq specified" << std::endl;
          return 1;
        }
        InStringStream str(argv[++arg]);
        str >> parameters.SoundParameters.ClockFreq;
      }
      else if (args == "--sound")
      {
        if (arg == argc - 1)
        {
          std::cout << "Invalid sound freq specified" << std::endl;
          return 1;
        }
        InStringStream str(argv[++arg]);
        str >> parameters.SoundParameters.SoundFreq;
      }
      else if (args == "--fir")
      {
        if (arg == argc - 1)
        {
          std::cout << "Invalid fir params specified" << std::endl;
        }
        InStringStream str(argv[++arg]);
        char tmp;
        str >> parameters.FIROrder >> tmp >> parameters.LowCutoff >> tmp >> parameters.HighCutoff;
      }
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

    IO::DataContainer::Ptr source(IO::DataContainer::Create(filename));
    ModulePlayer::Info playerInfo;
    if (!ModulePlayer::Check(filename, *source, playerInfo))
    {
      std::cerr << "Unsupported module type" << std::endl;
      return 1;
    }

    if (!backend.get())
    {
#ifdef _WIN32
      backend = Sound::CreateWinAPIBackend();
#else
      backend = Sound::CreateOSSBackend();
#endif
    }

    StringArray filesToPlay;
    {
      ModulePlayer::Ptr player(ModulePlayer::Create(filename, *source));
      Module::Information module;
      player->GetModuleInfo(module);
      if (module.Capabilities & CAP_MULTITRACK)
      {
        boost::algorithm::split(filesToPlay, module.Properties[Module::ATTR_SUBMODULES], boost::algorithm::is_cntrl());
      }
      else
      {
        filesToPlay.push_back(filename);
      }
    }

    boost::format formatter(
      "Position: %1$2d / %2% (%3%)\n"
      "Pattern:  %4$2d / %5%\n"
      "Line:     %6$2d / %7%\n"
      "Channels: %8$2d / %9%\n"
      "Tempo:    %10$2d / %11%\n"
      "Frame: %12$5d / %13%");

    std::size_t dump[100] = {0};
    bool quit(false);

    //generate mixers
    Sound::MixerData::Ptr mixer3(new Sound::MixerData);
    mixer3->Preamp = Sound::FIXED_POINT_PRECISION;
    mixer3->InMatrix.resize(3);
    mixer3->InMatrix[0].OutMatrix[0] = Sound::FIXED_POINT_PRECISION;
    mixer3->InMatrix[0].OutMatrix[1] = 5 * Sound::FIXED_POINT_PRECISION / 100;
    mixer3->InMatrix[1].OutMatrix[0] = 66 * Sound::FIXED_POINT_PRECISION / 100;
    mixer3->InMatrix[1].OutMatrix[1] = 66 * Sound::FIXED_POINT_PRECISION / 100;
    mixer3->InMatrix[2].OutMatrix[0] = 5 * Sound::FIXED_POINT_PRECISION / 100;
    mixer3->InMatrix[2].OutMatrix[1] = Sound::FIXED_POINT_PRECISION;

    Sound::MixerData::Ptr mixer4(new Sound::MixerData);
    mixer4->Preamp = Sound::FIXED_POINT_PRECISION;
    mixer4->InMatrix.resize(4);
    mixer4->InMatrix[0].OutMatrix[0] = Sound::FIXED_POINT_PRECISION;
    mixer4->InMatrix[0].OutMatrix[1] = 5 * Sound::FIXED_POINT_PRECISION / 100;
    mixer4->InMatrix[1].OutMatrix[0] = Sound::FIXED_POINT_PRECISION;
    mixer4->InMatrix[1].OutMatrix[1] = 5 * Sound::FIXED_POINT_PRECISION / 100;
    mixer4->InMatrix[2].OutMatrix[0] = 5 * Sound::FIXED_POINT_PRECISION / 100;
    mixer4->InMatrix[2].OutMatrix[1] = Sound::FIXED_POINT_PRECISION;
    mixer4->InMatrix[3].OutMatrix[0] = 5 * Sound::FIXED_POINT_PRECISION / 100;
    mixer4->InMatrix[3].OutMatrix[1] = Sound::FIXED_POINT_PRECISION;

    for (StringArray::const_iterator it = filesToPlay.begin(), lim = filesToPlay.end(); it != lim && !quit; ++it)
    {
      ModulePlayer::Ptr player(ModulePlayer::Create(*it, *source));
      if (!player.get())
      {
        continue;
      }
      Module::Information module;
      player->GetModuleInfo(module);
      backend->SetPlayer(player);
      Sound::Backend::Parameters params;
      backend->GetSoundParameters(params);

      params = parameters;

      switch (module.Statistic.Channels)
      {
      case 3:
        params.Mixer = mixer3;
        break;
      case 4:
        params.Mixer = mixer4;
        break;
      default:
        std::cerr << "Invalid channels count" << std::endl;
        return 1;
      }
      backend->SetSoundParameters(params);

      backend->Play();

      if (!silent)
      {
        std::cout << "Module: \n" << module.Properties;
      }

      for (;;)
      {
        const bool stop(Sound::Backend::STOPPED == backend->GetState());

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
          Sound::Analyze::ChannelsState state;
          backend->GetSoundState(state);
          const std::size_t WIDTH = 75;
          const std::size_t HEIGTH = 16;
          const std::size_t LIMIT = std::numeric_limits<Sound::Analyze::LevelType>::max();
          const std::size_t FALLSPEED = 8;
          static char filler[WIDTH + 1];
          for (std::size_t chan = 0; chan != state.size(); ++chan)
          {
            if (state[chan].Enabled)
            {
              dump[std::min(state.size() + 1 + state[chan].Band, ArraySize(dump) - 1)] =
                dump[chan] = state[chan].Level;
            }
          }
          for (std::size_t y = HEIGTH; y; --y)
          {
            for (std::size_t i = 0; i < std::min(WIDTH, ArraySize(dump)) - 1; ++i)
            {
              const std::size_t level(dump[i] * HEIGTH / LIMIT);
              filler[i] = level > y ? '#' : ' ';
              filler[i + 1] = 0;
            }
            std::cout << std::endl << filler;
          }
          std::transform(dump, ArrayEnd(dump), dump,
            std::bind2nd(std::ptr_fun(Decrease<Sound::Analyze::LevelType>), FALLSPEED));
          if (quit || stop)
          {
            std::cout << std::endl;
            break;
          }
          MoveUp(5 + HEIGTH);
        }
        boost::this_thread::sleep(boost::posix_time::milliseconds(20));
        const int key(tolower(GetKey()));
        quit = 'q' == key;
        if (quit || stop)
        {
          if (silent)//actual break after display
          {
            break;
          }
        }
        switch (key)
        {
        case 'p':
          Sound::Backend::STARTED == backend->GetState() ? backend->Pause() : backend->Play();
          break;
        case ' ':
          backend->Stop();
          break;
        case 'z':
          if (params.Mixer->Preamp)
          {
            --params.Mixer->Preamp;
          }
          break;
        case 'x':
          if (params.Mixer->Preamp < Sound::FIXED_POINT_PRECISION)
          {
            ++params.Mixer->Preamp;
          }
          break;
        }
      }
      backend->Stop();
    }
    return 0;
  }
  catch (const Error& e)
  {
    std::cout << e.Text << std::endl;
    return static_cast<int>(e.Code);
  }
}
