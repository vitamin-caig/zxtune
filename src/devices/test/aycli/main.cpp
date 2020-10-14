#include <api_dynamic.h>
#include <contract.h>
#include <devices/aym/chip.h>
#include <core/core_parameters.h>
#include <parameters/container.h>
#include <module/holder.h>
#include <sound/mixer_factory.h>
#include <sound/backends/storage.h>
#include <sound/backends/backends_list.h>
#include <sound/sound_parameters.h>
#include <iostream>

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
    virtual uint_t Version() const
    {
      return 0;
    }

    virtual uint64_t ClockFreq() const
    {
      return Parameters::ZXTune::Core::AYM::CLOCKRATE_DEFAULT;
    };

    virtual uint_t SoundFreq() const
    {
      return Parameters::ZXTune::Sound::FREQUENCY_DEFAULT;
    }

    virtual Devices::AYM::ChipType Type() const
    {
      return Devices::AYM::TYPE_AY38910;
    }

    virtual Devices::AYM::InterpolationType Interpolation() const
    {
      return Devices::AYM::INTERPOLATION_HQ;
    }

    virtual uint_t DutyCycleValue() const
    {
      return 50;
    }

    virtual uint_t DutyCycleMask() const
    {
      return 0;
    }

    virtual Devices::AYM::LayoutType Layout() const
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

  class StubRenderer : public Module::Renderer
  {
  public:
    StubRenderer(std::shared_ptr<Devices::AYM::DataChunk> chunk, Sound::Receiver::Ptr target)
      : Chunk(chunk)
      , Target(std::move(target))
      , Chip(Devices::AYM::CreateChip(StubChipParameters::Create(), CreateMixer()))
    {
    }

    virtual Module::State::Ptr GetState() const
    {
      return Module::State::Ptr();
    }

    virtual Module::Analyzer::Ptr GetAnalyzer() const
    {
       return Module::Analyzer::Ptr();
    }

    virtual bool RenderFrame(const Sound::LoopParameters&)
    {
      static const auto PERIOD = Time::Duration<Devices::AYM::TimeUnit>::FromFrequency(50);
      Chip->RenderData(*Chunk);
      Chunk->TimeStamp += PERIOD;
      Target->ApplyData(Chip->RenderTill(Chunk->TimeStamp));
      Target->Flush();
      Chunk->Data = Devices::AYM::Registers();
      return true;
    }

    virtual void Reset()
    {
      *Chunk = Devices::AYM::DataChunk();
    }

    virtual void SetPosition(Time::AtMillisecond /*request*/)
    {
    }
  private:
    const std::shared_ptr<Devices::AYM::DataChunk> Chunk;
    const Devices::AYM::Chip::Ptr Chip;
    const Sound::Receiver::Ptr Target;
  };

  class StubHolder : public Module::Holder
  {
  public:
    explicit StubHolder(std::shared_ptr<Devices::AYM::DataChunk> chunk)
      : Chunk(chunk)
    {
    }

    virtual Module::Information::Ptr GetModuleInformation() const
    {
      return Module::Information::Ptr();
    }

    virtual Parameters::Accessor::Ptr GetModuleProperties() const
    {
      return EmptyParams();
    }

    virtual Module::Renderer::Ptr CreateRenderer(Parameters::Accessor::Ptr /*params*/, Sound::Receiver::Ptr target) const
    {
      return std::make_shared<StubRenderer>(Chunk, target);
    }
  private:
    const std::shared_ptr<Devices::AYM::DataChunk> Chunk;
  };

  class BackendFactoryHandle : public Sound::BackendsStorage
  {
  public:
    virtual void Register(const String& /*id*/, const char* /*description*/, uint_t /*caps*/, Sound::BackendWorkerFactory::Ptr factory)
    {
      Factory = factory;
    }

    virtual void Register(const String& /*id*/, const char* /*description*/, uint_t /*caps*/, const Error& /*status*/)
    {
    }

    virtual void Register(const String& /*id*/, const char* /*description*/, uint_t /*caps*/)
    {
    }

    Sound::Backend::Ptr CreateBackend(Module::Holder::Ptr module)
    {
      const Sound::BackendWorker::Ptr worker = Factory->CreateWorker(module->GetModuleProperties(), module);
      return Sound::CreateBackend(module->GetModuleProperties(), module, Sound::BackendCallback::Ptr(), worker);
    }
  private:
    Sound::BackendWorkerFactory::Ptr Factory;
  };

  Sound::Backend::Ptr CreateBackend(std::shared_ptr<Devices::AYM::DataChunk> chunk)
  {
    const Module::Holder::Ptr module = std::make_shared<StubHolder>(chunk);
    BackendFactoryHandle factory;
    Sound::RegisterDirectSoundBackend(factory);
    return factory.CreateBackend(module);
  }
  
  class Gate
  {
  public:
    Gate()
      : Data(std::make_shared<Devices::AYM::DataChunk>())
      , Backend(CreateBackend(Data))
      , Control(Backend->GetPlaybackControl())
    {
    }
    
    bool IsStarted() const
    {
      return Sound::PlaybackControl::STARTED == Control->GetCurrentState();
    }
    
    void Start()
    {
      Control->Play();
    }
    
    void Stop()
    {
      Control->Stop();
    }
    
    void WriteReg(uint_t reg, uint_t val)
    {
      Require(reg < Devices::AYM::Registers::TOTAL);
      Require(val < 256);
      Data->Data[static_cast<Devices::AYM::Registers::Index>(reg)] = val;
    }
  private:
    const std::shared_ptr<Devices::AYM::DataChunk> Data;
    const Sound::Backend::Ptr Backend;
    const Sound::PlaybackControl::Ptr Control;
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
}

//dll part
#ifdef __cplusplus
extern "C" {
#endif
PUBLIC_API_EXPORT int ay_open();
PUBLIC_API_EXPORT void ay_close();
PUBLIC_API_EXPORT int ay_start();
PUBLIC_API_EXPORT int ay_stop();
PUBLIC_API_EXPORT int ay_writereg(int idx, int val);

#ifdef __cplusplus
} //extern
#endif

int ay_open()
{
  try
  {
    GateInstance.reset(new Gate());
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
  return call([idx, val](Gate* gate) {gate->WriteReg(idx, val);});
}

//exe part
int main(int /*argc*/, char* /*argv*/[])
{
  Gate gate;
  std::cout << "start/stop - playback control\nexit - finish\nw X Y - write to register X value Y\n";
  for (;;)
  {
    std::cout << (gate.IsStarted() ? "> " : "# ");
    std::string cmd;
    while (!(std::cin >> cmd));
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
