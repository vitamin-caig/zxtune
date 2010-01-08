#include <tools.h>
#include <formatter.h>
#include <core/player.h>
#include <core/devices/aym/aym.h>
#include <sound/backend.h>
#include <sound/error_codes.h>
#include <sound/render_params.h>

#include <iostream>
#include <iomanip>

#include <boost/thread.hpp>

namespace
{
  using namespace ZXTune;
  using namespace ZXTune::Sound;

  void ErrOuter(unsigned level, Error::LocationRef loc, Error::CodeType code, const String& text)
  {
    const String txt = (Formatter("\t%1%\n\tCode: %2%\n\tAt: %3%\n") % text % Error::CodeToString(code) % Error::LocationToString(loc)).str();
    if (level)
    {
      std::cout << "\t-------\n";
    }
    std::cout << txt;
  }
  
  bool ShowIfError(const Error& e)
  {
    if (e)
    {
      e.WalkSuberrors(ErrOuter);
    }
    return e;
  }

  void TestError(const char* msg, const Error& e)
  {
    const bool res(ShowIfError(e));
    std::cout << msg << " error test: " << (res ? "passed" : "failed") << "\n";
  }
  
  void TestSuccess(const char* msg, const Error& e)
  {
    const bool res(ShowIfError(e));
    std::cout << msg << " test: " << (res ? "failed" : "passed") << "\n";
  }
  
  class DummyHolder;
  Module::Player::Ptr CreateDummyPlayer(const DummyHolder& holder);
  
  class DummyHolder : public Module::Holder
  {
  public:
    DummyHolder() {}
    
    virtual void GetPlayerInfo(PluginInformation& info) const
    {
    }
    
    virtual void GetModuleInformation(Module::Information& info) const
    {
      info.PhysicalChannels = 3;
    }
    
    virtual Module::Player::Ptr CreatePlayer() const
    {
      return CreateDummyPlayer(*this);
    }

    virtual Error Convert(const Module::Conversion::Parameter& param, Dump& dst) const
    {
      return Error();
    }
  };

  
  class DummyPlayer : public Module::Player
  {
  public:
    explicit DummyPlayer(const DummyHolder& holder)
      : Holder(holder)
      , Chip(AYM::CreateChip())
      , Chunk()
      , Frames()
    {
      Chunk.Mask = AYM::DataChunk::ALL_REGISTERS ^ (1 << AYM::DataChunk::REG_TONEN);
      Chunk.Data[AYM::DataChunk::REG_MIXER] = ~7;
      Chunk.Data[AYM::DataChunk::REG_VOLA] = Chunk.Data[AYM::DataChunk::REG_VOLB] = Chunk.Data[AYM::DataChunk::REG_VOLC] = 15;
    }
        
    virtual const Module::Holder& GetModule() const
    {
      return Holder;
    }
    
    virtual Error GetPlaybackState(unsigned&, Module::Tracking&, Module::Analyze::ChannelsState&) const
    {
      return Error();
    }
    
    virtual Error RenderFrame(const RenderParameters& params, PlaybackState& state, MultichannelReceiver& receiver)
    {
      const uint16_t toneA((Frames + 1) * 1);
      const uint16_t toneB((Frames + 1) * 2);
      const uint16_t toneC((Frames + 1) * 3);
      Chunk.Data[AYM::DataChunk::REG_TONEA_L] = toneA & 0xff;
      Chunk.Data[AYM::DataChunk::REG_TONEA_H] = toneA >> 8;
      Chunk.Data[AYM::DataChunk::REG_TONEB_L] = toneB & 0xff;
      Chunk.Data[AYM::DataChunk::REG_TONEB_H] = toneB >> 8;
      Chunk.Data[AYM::DataChunk::REG_TONEC_L] = toneC & 0xff;
      Chunk.Data[AYM::DataChunk::REG_TONEC_H] = toneC >> 8;
      Chunk.Tick += params.ClocksPerFrame();
      Chip->RenderData(params, Chunk, receiver);
      Chunk.Mask &= ~(AYM::DataChunk::REG_VOLA | AYM::DataChunk::REG_VOLB | AYM::DataChunk::REG_VOLC);
      if (++Frames > 500)
      {
        state = MODULE_STOPPED;
      }
      else
      {
        state = MODULE_PLAYING;
      }
      return Error();
    }
    
    virtual Error Reset()
    {
      Chip->Reset();
      return Error();
    }
    
    virtual Error SetPosition(unsigned frame)
    {
      return Error();
    }
    
    virtual Error SetParameters(const Parameters::Map& params)
    {
      return Error();
    }
    
  private:
    const DummyHolder& Holder;
    const AYM::Chip::Ptr Chip;
    AYM::DataChunk Chunk;
    unsigned Frames;
  };
  
  Module::Player::Ptr CreateDummyPlayer(const DummyHolder& holder)
  {
    return Module::Player::Ptr(new DummyPlayer(holder));
  }
  

  void TestErrors()
  {
    using namespace ZXTune::Sound;
    Backend::Ptr backend;
    ThrowIfError(CreateBackend("null", backend));

    TestError("Play", backend->Play());
    TestError("Pause", backend->Pause());
    TestError("Stop", backend->Stop());
    TestError("SetPosition", backend->SetPosition(0));
    TestError("Null player set", backend->SetModule(Module::Holder::Ptr()));
  }

  void TestBackend(const BackendInfo& info)
  {
    std::cout << "Backend:\n"
    " Id: " << info.Id << "\n"
    " Version: " << info.Version << "\n"
    " Description: " << info.Description << "\n";

    Backend::Ptr backend;
    ThrowIfError(CreateBackend(info.Id, backend));
    
    std::cout << "Check for volume get: ";
    MultiGain volume;
    if (const Error& e = backend->GetVolume(volume))
    {
      if (e != BACKEND_UNSUPPORTED_FUNC)
      {
        throw e;
      }
      std::cout << "unsupported\n";
    }
    else
    {
      std::cout << "{" << volume[0] << "," << volume[1] << "}\n";
    }
    MultiGain newVol = { {1.0, 1.0} };
    std::cout << "Check for volume set: ";
    if (const Error& e = backend->SetVolume(newVol))
    {
      if (e != BACKEND_UNSUPPORTED_FUNC)
      {
        throw e;
      }
      std::cout << "unsuported\n";
    }
    else
    {
      std::cout << "passed\n";
    }
    //return previous
    backend->SetVolume(volume);
    TestSuccess("Player set", backend->SetModule(Module::Holder::Ptr(new DummyHolder())));
    /*
    TestSuccess("Play", backend->Play());
    TestSuccess("Pause", backend->Pause());
    TestSuccess("Resume", backend->Play());
    TestSuccess("Stop", backend->Stop());
    */
    std::cout << "Playing [";
    ThrowIfError(backend->Play());
    Backend::State state;
    for (;;)
    {
      boost::this_thread::sleep(boost::posix_time::milliseconds(1000));
      std::cout << '.' << std::flush;
      ThrowIfError(backend->GetCurrentState(state));
      if (state == Backend::STOPPED)
      {
        break;
      }
    }
    std::cout << "] stopped" << std::endl;
  }
}

int main()
{
  using namespace ZXTune;
  using namespace ZXTune::Sound;
  
  try
  {
    std::cout << "---- Test for error situations ---" << std::endl;
    TestErrors();

    std::cout << "---- Test for backends ---" << std::endl;
    BackendInfoArray infos;
    EnumerateBackends(infos);
    std::for_each(infos.begin(), infos.end(), TestBackend);
  }
  catch (const Error& e)
  {
    std::cout << " Failed\n";
    e.WalkSuberrors(ErrOuter);
  }
}
