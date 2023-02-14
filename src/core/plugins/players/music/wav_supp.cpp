/**
 *
 * @file
 *
 * @brief  WAV support plugin
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include "core/plugins/players/music/wav_supp.h"
#include "core/plugins/player_plugins_registrator.h"
#include "core/plugins/players/plugin.h"
// common includes
#include <contract.h>
#include <error_tools.h>
#include <make_ptr.h>
// library includes
#include <core/plugin_attrs.h>
#include <debug/log.h>
#include <formats/chiptune/decoders.h>
#include <formats/chiptune/music/wav.h>
#include <module/players/properties_helper.h>
#include <module/players/properties_meta.h>
#include <module/players/streaming.h>
#include <sound/resampler.h>

namespace Module::Wav
{
  const Debug::Stream Dbg("Core::WavSupp");

  class Renderer : public Module::Renderer
  {
  public:
    Renderer(Model::Ptr data, Sound::Converter::Ptr target)
      : Tune(std::move(data))
      , State(MakePtr<SampledState>(Tune->GetTotalSamples(), Tune->GetSamplerate()))
      , Target(std::move(target))
    {}

    Module::State::Ptr GetState() const override
    {
      return State;
    }

    Sound::Chunk Render(const Sound::LoopParameters& looped) override
    {
      if (!State->IsValid())
      {
        return {};
      }
      const auto loops = State->LoopCount();
      auto frame = Tune->RenderNextFrame();
      State->Consume(frame.size(), looped);
      if (State->LoopCount() != loops)
      {
        Tune->Seek(0);
      }
      return Target->Apply(std::move(frame));
    }

    void Reset() override
    {
      State->Reset();
      Tune->Seek(0);
    }

    void SetPosition(Time::AtMillisecond request) override
    {
      State->Seek(request);
      const auto aligned = Tune->Seek(State->AtSample());
      State->SeekAtSample(aligned);
    }

  private:
    const Model::Ptr Tune;
    const SampledState::Ptr State;
    const Sound::Converter::Ptr Target;
  };

  class Holder : public Module::Holder
  {
  public:
    Holder(Model::Ptr data, Parameters::Accessor::Ptr props)
      : Data(std::move(data))
      , Properties(std::move(props))
    {}

    Module::Information::Ptr GetModuleInformation() const override
    {
      return CreateSampledInfo(Data->GetSamplerate(), Data->GetTotalSamples());
    }

    Parameters::Accessor::Ptr GetModuleProperties() const override
    {
      return Properties;
    }

    Renderer::Ptr CreateRenderer(uint_t samplerate, Parameters::Accessor::Ptr /*params*/) const override
    {
      return MakePtr<Renderer>(Data, Sound::CreateResampler(Data->GetSamplerate(), samplerate));
    }

  private:
    const Model::Ptr Data;
    const Parameters::Accessor::Ptr Properties;
  };

  class DataBuilder : public Formats::Chiptune::Wav::Builder
  {
  public:
    explicit DataBuilder(PropertiesHelper& props)
      : Properties(props)
      , Meta(props)
    {}

    Formats::Chiptune::MetaBuilder& GetMetaBuilder() override
    {
      return Meta;
    }

    void SetProperties(uint_t formatCode, uint_t frequency, uint_t channels, uint_t bits, uint_t blockSize) override
    {
      FormatCode = formatCode;
      WavProperties.Frequency = frequency;
      WavProperties.Channels = channels;
      WavProperties.Bits = bits;
      WavProperties.BlockSize = blockSize;
    }

    void SetExtendedProperties(uint_t validBitsOrBlockSize, uint_t /*channelsMask*/,
                               const Formats::Chiptune::Wav::Guid& formatId, Binary::View restData) override
    {
      FormatId = formatId;
      WavProperties.BlockSizeSamples = validBitsOrBlockSize;
      ExtraData.resize(restData.Size());
      std::memcpy(ExtraData.data(), restData.Start(), restData.Size());
    }

    void SetExtraData(Binary::View data) override
    {
      ExtraData.resize(data.Size());
      std::memcpy(ExtraData.data(), data.Start(), data.Size());
    }

    void SetSamplesData(Binary::Container::Ptr data) override
    {
      WavProperties.Data = std::move(data);
    }

    void SetSamplesCountHint(uint_t count) override
    {
      WavProperties.SamplesCountHint = count;
    }

    Model::Ptr GetResult()
    {
      if (!WavProperties.Data)
      {
        return {};
      }
      switch (FormatCode)
      {
      case Formats::Chiptune::Wav::Format::PCM:
        return CreatePcmModel(WavProperties);
      case Formats::Chiptune::Wav::Format::IEEE_FLOAT:
        return CreateFloatPcmModel(WavProperties);
      case Formats::Chiptune::Wav::Format::ADPCM:
        return CreateAdpcmModel(WavProperties);
      case Formats::Chiptune::Wav::Format::IMA_ADPCM:
        return CreateImaAdpcmModel(WavProperties);
      case Formats::Chiptune::Wav::Format::ATRAC3:
        return CreateAtrac3Model(WavProperties, ExtraData);
      case Formats::Chiptune::Wav::Format::EXTENDED:
        return CreateExtendedModel();
      default:
        return {};
      }
    }

    Model::Ptr CreateExtendedModel()
    {
      Require(FormatCode == Formats::Chiptune::Wav::Format::EXTENDED);
      Require(FormatId != Formats::Chiptune::Wav::Guid());
      if (FormatId == Formats::Chiptune::Wav::ATRAC3PLUS)
      {
        return CreateAtrac3PlusModel(WavProperties);
      }
      else if (FormatId == Formats::Chiptune::Wav::ATRAC9)
      {
        return CreateAtrac9Model(WavProperties, ExtraData);
      }
      else
      {
        return {};
      }
    }

  private:
    PropertiesHelper& Properties;
    MetaProperties Meta;
    uint_t FormatCode = ~0;
    Formats::Chiptune::Wav::Guid FormatId;
    Wav::Properties WavProperties;
    Binary::Dump ExtraData;
  };

  class Factory : public Module::Factory
  {
  public:
    Module::Holder::Ptr CreateModule(const Parameters::Accessor& /*params*/, const Binary::Container& rawData,
                                     Parameters::Container::Ptr properties) const override
    {
      try
      {
        PropertiesHelper props(*properties);
        DataBuilder dataBuilder(props);
        if (const auto container = Formats::Chiptune::Wav::Parse(rawData, dataBuilder))
        {
          if (auto data = dataBuilder.GetResult())
          {
            props.SetSource(*container);
            return MakePtr<Holder>(std::move(data), std::move(properties));
          }
        }
      }
      catch (const std::exception& e)
      {
        Dbg("Failed to create WAV: {}", e.what());
      }
      return {};
    }
  };
}  // namespace Module::Wav

namespace ZXTune
{
  void RegisterWAVPlugin(PlayerPluginsRegistrator& registrator)
  {
    const Char ID[] = {'W', 'A', 'V', 0};
    const uint_t CAPS = Capabilities::Module::Type::STREAM | Capabilities::Module::Device::DAC;

    const auto decoder = Formats::Chiptune::CreateWAVDecoder();
    const auto factory = MakePtr<Module::Wav::Factory>();
    const PlayerPlugin::Ptr plugin = CreatePlayerPlugin(ID, CAPS, decoder, factory);
    registrator.RegisterPlugin(plugin);
  }
}  // namespace ZXTune

