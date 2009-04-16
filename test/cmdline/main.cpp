#include <sound.h>
#include <player.h>
#include <sound_attrs.h>

#include <../../lib/sound/mixer.h>
#include <../../lib/sound/renderer.h>

#include <tools.h>
#include <error.h>

#include <fstream>
#include <iostream>

#include "sdl/include/SDL.h"

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

  template<std::size_t Channels>
  class AutoSDL : private ZXTune::Sound::Receiver
  {
    class Mutex
    {
    public:
      Mutex() : Locker(SDL_CreateMutex())
      {
      }

      ~Mutex()
      {
        Unlock();
        SDL_DestroyMutex(Locker);
      }

      void Lock()
      {
        SDL_mutexP(Locker);
      }
      void Unlock()
      {
        SDL_mutexV(Locker);
      }
    private:
      SDL_mutex* Locker;
    };

    class AutoLocker
    {
    public:
      AutoLocker(Mutex& mtx) : Obj(mtx)
      {
        Obj.Lock();
      }
      ~AutoLocker()
      {
        Obj.Unlock();
      }
    private:
      Mutex& Obj;
    };
  public:
    AutoSDL(const ZXTune::Sound::Parameters& params)
    {
      SDL_AudioSpec fmt;
      fmt.freq = params.SoundFreq;
      fmt.format = AUDIO_U16;
      fmt.channels = Channels;
      fmt.samples = 8192;
      fmt.callback = OnSound;
      fmt.userdata = this;

      if (SDL_OpenAudio(&fmt, 0) < 0)
      {
        throw std::runtime_error("Failed to open audio");
      }
    }

    virtual ~AutoSDL()
    {
      SDL_CloseAudio();
    }

    void Play(ZXTune::Player* player)
    {
      SDL_PauseAudio(0);
      
    }
  private:
    static void OnSound(void* user, Uint8* stream, int len)
    {
      static_cast<AutoSDL*>(user)->GetData(stream, len);
    }

    void GetData(void* data, int size)
    {
    }

    void ApplySample(const Sample* input, std::size_t channels)
    {
      assert(Writer.get());
      Writer->ApplySample(input, channels);
    }
  private:
    ZXTune::Sound::Receiver::Ptr Writer;
    int PrevSize;
    void* PrevData;
    ZXTune::Player* Player;
  }; 

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

    ZXTune::Sound::Sample mixMap[4][2] = {{100, 10}, {60, 60}, {10, 100}, {50, 50}};
    std::ofstream file((filename + ".wav").c_str(), std::ios::binary);
    file.write(safe_ptr_cast<char*>(&header[0]), sizeof(header));
    ZXTune::Sound::Receiver::Ptr renderer(ZXTune::Sound::CreateRenderer<2>(4096, &Writer, static_cast<std::ostream*>(&file)));
    ZXTune::Sound::Receiver::Ptr mixer(ZXTune::Sound::CreateMixer(mixMap, renderer.get()));
    while (ZXTune::Player::MODULE_STOPPED != player->RenderFrame(sndParams, mixer.get()));
    return 0;
  }
  catch (const Error& e)
  {
    return static_cast<int>(e.Code);
  }
}
