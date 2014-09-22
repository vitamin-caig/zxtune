#include <devices/aym/chip.h>
#include <core/module_holder.h>
#include <core/module_player.h>
#include <core/core_parameters.h>
#include <parameters/container.h>
#include <sound/mixer_factory.h>
#include <sound/backends/storage.h>
#include <sound/backends/backends_list.h>
#include <sound/sound_parameters.h>
#include <boost/make_shared.hpp>
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
    StubRenderer(boost::shared_ptr<Devices::AYM::DataChunk> chunk, Sound::Receiver::Ptr target)
      : Chunk(chunk)
      , Chip(Devices::AYM::CreateChip(StubChipParameters::Create(), CreateMixer(), target))
    {
    }

    virtual Module::TrackState::Ptr GetTrackState() const
    {
      return Module::TrackState::Ptr();
    }

    virtual Module::Analyzer::Ptr GetAnalyzer() const
    {
       return Module::Analyzer::Ptr();
    }

    virtual bool RenderFrame()
    {
      static const Devices::AYM::Stamp PERIOD(Parameters::ZXTune::Sound::FRAMEDURATION_DEFAULT);
      Chunk->TimeStamp += PERIOD;
      Chip->RenderData(*Chunk);
      Chunk->Data = Devices::AYM::Registers();
      return true;
    }

    virtual void Reset()
    {
      *Chunk = Devices::AYM::DataChunk();
    }

    virtual void SetPosition(uint_t /*frame*/)
    {
    }
  private:
    const boost::shared_ptr<Devices::AYM::DataChunk> Chunk;
    const Devices::AYM::Chip::Ptr Chip;
  };

  class StubHolder : public Module::Holder
  {
  public:
    explicit StubHolder(boost::shared_ptr<Devices::AYM::DataChunk> chunk)
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
      return boost::make_shared<StubRenderer>(Chunk, target);
    }
  private:
    const boost::shared_ptr<Devices::AYM::DataChunk> Chunk;
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
      const Sound::BackendWorker::Ptr worker = Factory->CreateWorker(module->GetModuleProperties());
      return Sound::CreateBackend(module->GetModuleProperties(), module, Sound::BackendCallback::Ptr(), worker);
    }
  private:
    Sound::BackendWorkerFactory::Ptr Factory;
  };

  Sound::Backend::Ptr CreateBackend(boost::shared_ptr<Devices::AYM::DataChunk> chunk)
  {
    const Module::Holder::Ptr module = boost::make_shared<StubHolder>(chunk);
    BackendFactoryHandle factory;
    Sound::RegisterDirectSoundBackend(factory);
    return factory.CreateBackend(module);
  }
}

int main(int /*argc*/, char* /*argv*/[])
{
  const boost::shared_ptr<Devices::AYM::DataChunk> data = boost::make_shared<Devices::AYM::DataChunk>();
  const Sound::Backend::Ptr backend = CreateBackend(data);
  const Sound::PlaybackControl::Ptr ctrl = backend->GetPlaybackControl();
  std::cout << "start/stop - playback control\nexit - finish\nw X Y - write to register X value Y\n";
  for (;;)
  {
    std::cout << (Sound::PlaybackControl::STARTED == ctrl->GetCurrentState() ? "> " : "# ");
    std::string cmd;
    std::cin >> cmd;
    if (cmd == "exit")
    {
      break;
    }
    else if (cmd == "start")
    {
      ctrl->Play();
    }
    else if (cmd == "stop")
    {
      ctrl->Stop();
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
        data->Data[static_cast<Devices::AYM::Registers::Index>(reg)] = val;
      }
    }
  }
}
