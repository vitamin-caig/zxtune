#include <tools.h>
#include <core/player.h>
#include <src/sound/backend.h>
#include <src/sound/error_codes.h>

#include <iostream>
#include <iomanip>

#include <boost/thread.hpp>

namespace
{
  using namespace ZXTune;
  using namespace ZXTune::Sound;

  void ErrOuter(unsigned level, Error::LocationRef loc, Error::CodeType code, const String& text)
  {
    const String txt = (Formatter("\t%1%\n\tCode: %2$#x\n\tAt: %3%\n") % text % code % Error::LocationToString(loc)).str();
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

  void ShowBackend(const Backend::Info& info)
  {
    std::cout << "Backend:\n"
    " Id: " << info.Id << "\n"
    " Version: " << info.Version << "\n"
    " Description: " << info.Description << "\n"
    "---------------------\n";
  }
  
  void TestError(const char* msg, const Error& e)
  {
    const bool res(ShowIfError(e));
    std::cout << msg << " error test: " << (res ? "passed" : "failed") << "\n\n";
  }
  
  void TestSuccess(const char* msg, const Error& e)
  {
    const bool res(ShowIfError(e));
    std::cout << msg << " test: " << (res ? "failed" : "passed") << "\n\n";
  }
  
  class DummyPlayer : public Module::Player
  {
  public:
    DummyPlayer() : Frames()
    {
    }
    
    virtual void GetPlayerInfo(PluginInformation& info) const
    {
    }
    
    virtual void GetModuleInformation(Module::Information& info) const
    {
      info.PhysicalChannels = 1;
    }
    
    virtual Error GetModuleState(unsigned&, Module::Tracking&, Module::Analyze::ChannelsState&) const
    {
      return Error();
    }
    
    virtual Error RenderFrame(const RenderParameters& params, PlaybackState& state, MultichannelReceiver& receiver)
    {
      static const std::vector<Sample> sample(1);
      receiver.ApplySample(sample);
      state = ++Frames > 1000 ? MODULE_STOPPED : MODULE_PLAYING;
      return Error();
    }
    
    virtual Error Reset()
    {
      return Error();
    }
    
    virtual Error SetPosition(unsigned frame)
    {
      return Error();
    }
    
    virtual Error Convert(const Module::Conversion::Parameter& param, Dump& dst) const
    {
      return Error();
    }
  private:
    unsigned Frames;
  };
}

int main()
{
  using namespace ZXTune;
  using namespace ZXTune::Sound;
  
  try
  {
    std::cout << "---- Test for backends enumerating ---" << std::endl;
    std::vector<Backend::Info> infos;
    EnumerateBackends(infos);
    std::for_each(infos.begin(), infos.end(), ShowBackend);
    
    Backend::Ptr backend;
    ThrowIfError(CreateBackend("null", backend));
    
    TestError("Play", backend->Play());
    TestError("Pause", backend->Pause());
    TestError("Stop", backend->Stop());
    TestError("SetPosition", backend->SetPosition(0));
    TestError("Null player set", backend->SetPlayer(Module::Player::Ptr()));
    TestSuccess("Player set", backend->SetPlayer(Module::Player::Ptr(new DummyPlayer)));
    TestSuccess("Play", backend->Play());
    TestSuccess("Pause", backend->Pause());
    TestSuccess("Resume", backend->Play());
    TestSuccess("Stop", backend->Stop());
    ThrowIfError(backend->Play());
    boost::this_thread::sleep(boost::posix_time::milliseconds(5000));
  }
  catch (const Error& e)
  {
    std::cout << " Failed\n";
    e.WalkSuberrors(ErrOuter);
  }
}
