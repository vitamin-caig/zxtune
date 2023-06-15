#include <api_dynamic.h>
#include <async/worker.h>
#include <contract.h>
#include <core/core_parameters.h>
#include <devices/aym/chip.h>
#include <iostream>
#include <make_ptr.h>
#include <memory>
#include <mutex>
#include <parameters/container.h>
#include <sound/backends/backends_list.h>
#include <sound/backends/storage.h>
#include <sound/mixer_factory.h>
#include <sound/sound_parameters.h>
#include <utility>

namespace
{
  Parameters::Accessor::Ptr EmptyParams()
  {
    static const Parameters::Accessor::Ptr INSTANCE = Parameters::Container::Create();
    return INSTANCE;
  }

  class StubChipParameters : public Devices::AYM::ChipParameters
  {
  public:
    uint_t Version() const override
    {
      return 0;
    }

    uint64_t ClockFreq() const override
    {
      return Parameters::ZXTune::Core::AYM::CLOCKRATE_DEFAULT;
    };

    uint_t SoundFreq() const override
    {
      return Parameters::ZXTune::Sound::FREQUENCY_DEFAULT;
    }

    Devices::AYM::ChipType Type() const override
    {
      return Devices::AYM::TYPE_AY38910;
    }

    Devices::AYM::InterpolationType Interpolation() const override
    {
      return Devices::AYM::INTERPOLATION_HQ;
    }

    uint_t DutyCycleValue() const override
    {
      return 50;
    }

    uint_t DutyCycleMask() const override
    {
      return 0;
    }

    Devices::AYM::LayoutType Layout() const override
    {
      return Devices::AYM::LAYOUT_ABC;
    }

    static Ptr Create()
    {
      static StubChipParameters INSTANCE;
      return MakeSingletonPointer(INSTANCE);
    }
  };

  Devices::AYM::MixerType::Ptr CreateMixer()
  {
    const Sound::ThreeChannelsMatrixMixer::Ptr mixer = Sound::ThreeChannelsMatrixMixer::Create();
    Sound::FillMixer(*EmptyParams(), *mixer);
    return mixer;
  }

  class StubState : public Module::State
  {
  public:
    Time::AtMillisecond At() const override
    {
      return {};
    }

    Time::Milliseconds Total() const override
    {
      return {};
    }

    uint_t LoopCount() const override
    {
      return 0;
    }
  };

  class State
  {
  public:
    using Ptr = std::shared_ptr<State>;

    void Write(uint_t reg, uint_t val)
    {
      const std::unique_lock<std::mutex> lock(Mutex);
      Require(reg < Devices::AYM::Registers::TOTAL);
      Require(val < 256);
      Data[static_cast<Devices::AYM::Registers::Index>(reg)] = val;
    }

    void Read(Devices::AYM::Registers* out)
    {
      const std::unique_lock<std::mutex> lock(Mutex);
      *out = Data;
      Data = {};
    }

  private:
    std::mutex Mutex;
    Devices::AYM::Registers Data;
  };

  class AsyncWorker : public Async::Worker
  {
  public:
    AsyncWorker(State::Ptr state, Sound::BackendWorker::Ptr worker)
      : Data(std::move(state))
      , Worker(std::move(worker))
      , Chip(Devices::AYM::CreateChip(StubChipParameters::Create(), CreateMixer()))
    {}

    void Initialize() override
    {
      Worker->Startup();
    }

    void Finalize() override
    {
      Worker->Shutdown();
    }

    void Suspend() override
    {
      Worker->Pause();
    }

    void Resume() override
    {
      Worker->Resume();
    }

    bool IsFinished() const override
    {
      return false;
    }

    void ExecuteCycle() override
    {
      static const StubState STATE;
      static const auto PERIOD = Time::Duration<Devices::AYM::TimeUnit>::FromFrequency(50);
      Worker->FrameStart(STATE);
      Data->Read(&Chunk.Data);
      Chip->RenderData(Chunk);
      Chunk.TimeStamp += PERIOD;
      Worker->FrameFinish(Chip->RenderTill(Chunk.TimeStamp));
    }

  private:
    const State::Ptr Data;
    const Sound::BackendWorker::Ptr Worker;
    const Devices::AYM::Chip::Ptr Chip;
    Devices::AYM::DataChunk Chunk;
  };

  class BackendFactoryHandle : public Sound::BackendsStorage
  {
  public:
    virtual void Register(const String& /*id*/, const char* /*description*/, uint_t /*caps*/,
                          Sound::BackendWorkerFactory::Ptr factory)
    {
      Factory = std::move(factory);
    }

    virtual void Register(const String& /*id*/, const char* /*description*/, uint_t /*caps*/, const Error& /*status*/)
    {}

    virtual void Register(const String& /*id*/, const char* /*description*/, uint_t /*caps*/) {}

    Async::Job::Ptr CreatePlayer(State::Ptr state)
    {
      auto backendWorker = Factory->CreateWorker(EmptyParams(), {});
      auto asyncWorker = MakePtr<AsyncWorker>(std::move(state), std::move(backendWorker));
      return Async::CreateJob(std::move(asyncWorker));
    }

  private:
    Sound::BackendWorkerFactory::Ptr Factory;
  };

  Async::Job::Ptr CreatePlayer(const State::Ptr& state)
  {
    BackendFactoryHandle factory;
    Sound::RegisterDirectSoundBackend(factory);
    return factory.CreatePlayer(std::move(state));
  }

  class Gate
  {
  public:
    Gate()
      : Data(MakePtr<State>())
      , Player(CreatePlayer(Data))
    {}

    bool IsStarted() const
    {
      return Player->IsActive();
    }

    void Start()
    {
      Player->Start();
    }

    void Stop()
    {
      Player->Stop();
    }

    void WriteReg(uint_t reg, uint_t val)
    {
      Data->Write(reg, val);
    }

  private:
    const State::Ptr Data;
    const Async::Job::Ptr Player;
  };

  std::unique_ptr<Gate> GateInstance;

  template<class Op>
  int call(Op op)
  {
    try
    {
      if (Gate* gate = GateInstance.get())
      {
        op(gate);
        return 0;
      }
      else
      {
        return -1;
      }
    }
    catch (const Error& e)
    {
      std::cerr << e.ToString();
      return 1;
    }
    catch (const std::exception&)
    {
      return 1;
    }
  }
}  // namespace

// dll part
#ifdef __cplusplus
extern "C"
{
#endif
  PUBLIC_API_EXPORT int ay_open();
  PUBLIC_API_EXPORT void ay_close();
  PUBLIC_API_EXPORT int ay_start();
  PUBLIC_API_EXPORT int ay_stop();
  PUBLIC_API_EXPORT int ay_writereg(int idx, int val);

#ifdef __cplusplus
}  // extern
#endif

int ay_open()
{
  try
  {
    GateInstance = std::make_unique<Gate>();
    return 0;
  }
  catch (const Error& e)
  {
    std::cerr << e.ToString();
    return 1;
  }
}

void ay_close()
{
  GateInstance.reset();
}

int ay_start()
{
  return call(std::mem_fun(&Gate::Start));
}

int ay_stop()
{
  return call(std::mem_fun(&Gate::Stop));
}

int ay_writereg(int idx, int val)
{
  return call([idx, val](Gate* gate) { gate->WriteReg(idx, val); });
}

// exe part
int main(int /*argc*/, char* /*argv*/[])
{
  Gate gate;
  std::cout << "start/stop - playback control\nexit - finish\nw X Y - write to register X value Y\n";
  for (;;)
  {
    std::cout << (gate.IsStarted() ? "> " : "# ");
    std::string cmd;
    while (!(std::cin >> cmd))
      ;
    if (cmd == "exit")
    {
      break;
    }
    else if (cmd == "start")
    {
      gate.Start();
    }
    else if (cmd == "stop")
    {
      gate.Stop();
    }
    else if (cmd == "w")
    {
      unsigned reg = 0, val = 0;
      std::cin >> reg >> val;
      if (reg >= Devices::AYM::Registers::TOTAL)
      {
        std::cout << "Invalid register index (" << reg << ")\n";
      }
      else if (val > 255)
      {
        std::cout << "Invalid register value (" << val << ")\n";
      }
      else
      {
        gate.WriteReg(reg, val);
      }
    }
  }
}
