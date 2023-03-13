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
#include <math/numeric.h>
#include <module/players/properties_helper.h>
#include <module/players/properties_meta.h>
#include <module/players/streaming.h>
#include <sound/resampler.h>
// 3rdparty
#define OV_EXCLUDE_STATIC_CALLBACKS
#include <3rdparty/vorbis/vorbisfile.h>
// std includes
#include <algorithm>
#include <unordered_map>

namespace Module::Ogg
{
  const Debug::Stream Dbg("Core::OggSupp");

  struct Model
  {
    using RWPtr = std::shared_ptr<Model>;
    using Ptr = std::shared_ptr<const Model>;

    uint_t Frequency = 0;
    uint64_t TotalSamples = 0;
    Binary::Data::Ptr Content;
  };

  class VorbisDecoder
  {
  public:
    using Ptr = std::unique_ptr<VorbisDecoder>;

    explicit VorbisDecoder(Binary::View data)
      : Data(data)
    {
      const ov_callbacks cb = {.read_func = &DoRead, .seek_func = &DoSeek, .close_func = nullptr, .tell_func = &DoTell};
      const auto res = ::ov_open_callbacks(this, &File, nullptr, 0, cb);
      if (res != 0)
      {
        throw MakeFormattedError(THIS_LINE, "Failed to open OGG Vorbis: {}", res);
      }
      Channels = ::ov_info(&File, -1)->channels;
    }

    ~VorbisDecoder()
    {
      ::ov_clear(&File);
    }

    void Seek(uint64_t sample)
    {
      const auto res = ::ov_pcm_seek_lap(&File, sample);
      if (res != 0)
      {
        throw MakeFormattedError(THIS_LINE, "Failed to seek OGG stream: {}", res);
      }
    }

    std::size_t Render(Sound::Sample* target, std::size_t avail)
    {
      std::size_t done = 0;
      while (done < avail)
      {
        float** pcm = nullptr;
        const auto res = ::ov_read_float(&File, &pcm, std::min(2048, int(avail - done)), nullptr);
        if (res == 0)
        {
          break;
        }
        else if (res < 0)
        {
          continue;
        }
        Convert(pcm, target + done, res);
        done += res;
      }
      return done;
    }

  private:
    static size_t DoRead(void* target, size_t size, size_t count, void* ctx)
    {
      auto* const self = static_cast<VorbisDecoder*>(ctx);
      const auto avail = (self->Data.Size() - self->Position) / size;
      const auto toRead = std::min(avail, count);
      if (toRead)
      {
        std::memcpy(target, self->Data.As<uint8_t>() + self->Position, toRead * size);
        self->Position += toRead * size;
      }
      return toRead;
    }

    static int DoSeek(void* ctx, ogg_int64_t offset, int whence)
    {
      auto* const self = static_cast<VorbisDecoder*>(ctx);
      std::size_t newPos = offset;
      switch (whence)
      {
      case SEEK_SET:
        break;
      case SEEK_CUR:
        newPos += self->Position;
        break;
      case SEEK_END:
        newPos = self->Data.Size();
        break;
      default:
        return -1;
      }
      if (newPos <= self->Data.Size())
      {
        self->Position = newPos;
        return 0;
      }
      else
      {
        return -1;
      }
    }

    static long DoTell(void* ctx)
    {
      auto* const self = static_cast<VorbisDecoder*>(ctx);
      return self->Position;
    }

    void Convert(float** in, Sound::Sample* out, std::size_t samples)
    {
      static_assert(Sound::Sample::CHANNELS == 2, "Incompatible sound sample channels count");
      static_assert(Sound::Sample::BITS == 16, "Incompatible sound sample bits count");
      static_assert(Sound::Sample::MID == 0, "Incompatible sound sample type");
      if (Channels == 1)
      {
        std::transform(in[0], in[0] + samples, out, &MakeMonoSample);
      }
      else
      {
        std::transform(in[0], in[0] + samples, in[1], out, &MakeStereoSample);
      }
    }

    inline static Sound::Sample::Type MakeSample(float s)
    {
      const auto wide = static_cast<Sound::Sample::WideType>(s * Sound::Sample::MAX);
      return Math::Clamp<Sound::Sample::WideType>(wide, Sound::Sample::MIN, Sound::Sample::MAX);
    }

    inline static Sound::Sample MakeMonoSample(float f)
    {
      const auto smp = MakeSample(f);
      return {smp, smp};
    }

    inline static Sound::Sample MakeStereoSample(float l, float r)
    {
      return {MakeSample(l), MakeSample(r)};
    }

  private:
    const Binary::View Data;
    std::size_t Position = 0;
    OggVorbis_File File;
    int Channels = 0;
  };

  class OggTune
  {
  public:
    explicit OggTune(Model::Ptr data)
      : Data(std::move(data))
    {
      Reset();
    }

    uint_t GetFrequency() const
    {
      return Data->Frequency;
    }

    Sound::Chunk RenderFrame()
    {
      Sound::Chunk chunk(Data->Frequency / 10);
      const auto done = Decoder->Render(chunk.data(), chunk.size());
      chunk.resize(done);
      return chunk;
    }

    void Reset()
    {
      Decoder = MakePtr<VorbisDecoder>(*Data->Content);
    }

    void Seek(uint64_t sample)
    {
      Decoder->Seek(sample);
    }

  private:
    const Model::Ptr Data;
    VorbisDecoder::Ptr Decoder;
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

    Sound::Chunk Render() override
    {
      auto frame = Tune.RenderFrame();
      if (0 != State->Consume(frame.size()))
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

    void AddUnknownPacket(Binary::View /*data*/) override {}

    void SetProperties(uint_t /*channels*/, uint_t frequency, uint_t /*blockSizeLo*/, uint_t /*blockSizeHi*/) override
    {
      AllocateStream()->Frequency = frequency;
    }

    void SetSetup(Binary::View /*data*/) override {}

    void AddFrame(std::size_t /*offset*/, uint64_t positionInFrames, Binary::View /*data*/) override
    {
      if (CurrentStream)
      {
        CurrentStream->TotalSamples = positionInFrames;
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
