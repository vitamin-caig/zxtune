/**
 *
 * @file
 *
 * @brief  OGG support plugin
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
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
#include <formats/chiptune/music/oggvorbis.h>
#include <module/players/properties_helper.h>
#include <module/players/properties_meta.h>
#include <module/players/streaming.h>
#include <sound/resampler.h>
// 3rdparty
#define STB_VORBIS_NO_STDIO
//#define STB_VORBIS_NO_PUSHDATA_API
//#define STB_VORBIS_MAX_CHANNELS 2
#if BOOST_ENDIAN_BIG_BYTE
#  define STB_VORBIS_BIG_ENDIAN
#endif
#include <3rdparty/stb/stb_vorbis.c>
// std includes
#include <unordered_map>

#define FILE_TAG B064CB04

namespace Module::Ogg
{
  const Debug::Stream Dbg("Core::OggSupp");

  struct Model
  {
    using RWPtr = std::shared_ptr<Model>;
    using Ptr = std::shared_ptr<const Model>;

    uint_t Frequency = 0;
    uint_t TotalSamples = 0;
    Binary::Data::Ptr Content;
  };

  class OggTune
  {
  public:
    explicit OggTune(Model::Ptr data)
      : Data(std::move(data))
    {
      Reset();
      static_assert(Sound::Sample::BITS == 16, "Incompatible sound sample bits count");
      static_assert(Sound::Sample::MID == 0, "Incompatible sound sample type");
    }

    uint_t GetFrequency() const
    {
      return Data->Frequency;
    }

    Sound::Chunk RenderFrame()
    {
      Sound::Chunk chunk(Data->Frequency / 10);
      const auto done = ::stb_vorbis_get_samples_short_interleaved(Decoder.get(), Sound::Sample::CHANNELS,
                                                                   safe_ptr_cast<short*>(chunk.data()),
                                                                   int(Sound::Sample::CHANNELS * chunk.size()));
      chunk.resize(done);
      return chunk;
    }

    void Reset()
    {
      int error = 0;
      const auto decoder = VorbisPtr(::stb_vorbis_open_memory(static_cast<const uint8_t*>(Data->Content->Start()),
                                                              int(Data->Content->Size()), &error, nullptr),
                                     &::stb_vorbis_close);
      if (!decoder)
      {
        throw MakeFormattedError(THIS_LINE, "Failed to create decoder. Error: {}", error);
      }
      Decoder = decoder;
    }

    void Seek(uint64_t sample)
    {
      if (!::stb_vorbis_seek(Decoder.get(), sample))
      {
        throw Error(THIS_LINE, "Failed to seek");
      }
    }

  private:
    const Model::Ptr Data;
    using VorbisPtr = std::shared_ptr<stb_vorbis>;
    VorbisPtr Decoder;
  };

  class Renderer : public Module::Renderer
  {
  public:
    Renderer(Model::Ptr data, Sound::Converter::Ptr target)
      : Tune(data)
      , State(MakePtr<SampledState>(data->TotalSamples, data->Frequency))
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
      auto frame = Tune.RenderFrame();
      State->Consume(frame.size(), looped);
      if (State->LoopCount() != loops)
      {
        Tune.Seek(0);
      }
      return Target->Apply(std::move(frame));
    }

    void Reset() override
    {
      Tune.Reset();
      State->Reset();
    }

    void SetPosition(Time::AtMillisecond request) override
    {
      State->Seek(request);
      Tune.Seek(State->AtSample());
    }

  private:
    OggTune Tune;
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
      return CreateSampledInfo(Data->Frequency, Data->TotalSamples);
    }

    Parameters::Accessor::Ptr GetModuleProperties() const override
    {
      return Properties;
    }

    Renderer::Ptr CreateRenderer(uint_t samplerate, Parameters::Accessor::Ptr /*params*/) const override
    {
      return MakePtr<Renderer>(Data, Sound::CreateResampler(Data->Frequency, samplerate));
    }

  private:
    const Model::Ptr Data;
    const Parameters::Accessor::Ptr Properties;
  };

  class DataBuilder : public Formats::Chiptune::OggVorbis::Builder
  {
  public:
    explicit DataBuilder(PropertiesHelper& props)
      : Meta(props)
    {}

    Formats::Chiptune::MetaBuilder& GetMetaBuilder() override
    {
      return Meta;
    }

    void SetStreamId(uint32_t id) override
    {
      CurrentStreamId = id;
      const auto existing = Streams.find(id);
      if (existing != Streams.end())
      {
        CurrentStream = existing->second.get();
      }
      else
      {
        CurrentStream = nullptr;
      }
    }

    void AddUnknownPacket(Binary::View data) override {}

    void SetProperties(uint_t /*channels*/, uint_t frequency, uint_t /*blockSizeLo*/, uint_t /*blockSizeHi*/) override
    {
      AllocateStream()->Frequency = frequency;
    }

    void SetSetup(Binary::View /*data*/) override {}

    void AddFrame(std::size_t /*offset*/, uint_t samples, Binary::View /*data*/) override
    {
      if (CurrentStream)
      {
        CurrentStream->TotalSamples += samples;
      }
      else
      {
        Dbg("Ignore frame to unallocated stream {}", CurrentStreamId);
      }
    }

    void SetContent(Binary::Data::Ptr data)
    {
      for (auto& stream : Streams)
      {
        stream.second->Content = data;
      }
    }

    Model::Ptr GetResult()
    {
      if (Streams.size() > 1)
      {
        Dbg("Multistream file with {} streams", Streams.size());
      }
      if (DefaultStream && DefaultStream->TotalSamples)
      {
        return DefaultStream;
      }
      else
      {
        return {};
      }
    }

  private:
    Model* AllocateStream()
    {
      Require(0 == Streams.count(CurrentStreamId));
      auto stream = MakeRWPtr<Model>();
      if (!DefaultStream)
      {
        DefaultStream = stream;
      }
      CurrentStream = stream.get();
      Streams.emplace(CurrentStreamId, std::move(stream));
      return CurrentStream;
    }

  private:
    std::unordered_map<uint32_t, Model::RWPtr> Streams;
    uint_t CurrentStreamId = 0;
    Model* CurrentStream = nullptr;
    Model::Ptr DefaultStream;
    MetaProperties Meta;
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
        if (const auto container = Formats::Chiptune::OggVorbis::Parse(rawData, dataBuilder))
        {
          if (auto data = dataBuilder.GetResult())
          {
            props.SetSource(*container);
            dataBuilder.SetContent(container);
            return MakePtr<Holder>(std::move(data), std::move(properties));
          }
        }
      }
      catch (const std::exception& e)
      {
        Dbg("Failed to create OGG: {}", e.what());
      }
      return {};
    }
  };
}  // namespace Module::Ogg

namespace ZXTune
{
  void RegisterOGGPlugin(PlayerPluginsRegistrator& registrator)
  {
    const Char ID[] = {'O', 'G', 'G', 0};
    const uint_t CAPS = Capabilities::Module::Type::STREAM | Capabilities::Module::Device::DAC;

    const auto decoder = Formats::Chiptune::CreateOGGDecoder();
    const auto factory = MakePtr<Module::Ogg::Factory>();
    const PlayerPlugin::Ptr plugin = CreatePlayerPlugin(ID, CAPS, decoder, factory);
    registrator.RegisterPlugin(plugin);
  }
}  // namespace ZXTune

#undef FILE_TAG
