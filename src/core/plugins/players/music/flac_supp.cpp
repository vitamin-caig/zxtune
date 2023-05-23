/**
 *
 * @file
 *
 * @brief  FLAC support plugin
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
#include <binary/input_stream.h>
#include <core/plugin_attrs.h>
#include <debug/log.h>
#include <formats/chiptune/decoders.h>
#include <formats/chiptune/music/flac.h>
#include <module/players/properties_helper.h>
#include <module/players/properties_meta.h>
#include <module/players/streaming.h>
#include <sound/resampler.h>
// 3rdparty
#define FLAC__NO_DLL
#include <3rdparty/FLAC/stream_decoder.h>

namespace Module::Flac
{
  const Debug::Stream Dbg("Core::FlacSupp");

  struct Model
  {
    using RWPtr = std::shared_ptr<Model>;
    using Ptr = std::shared_ptr<const Model>;

    uint_t Frequency = 0;
    uint_t TotalSamples = 0;
    uint_t MaxFrameSize = 0;
    Binary::Data::Ptr Content;
  };

  template<uint_t width>
  struct SampleTraits;

  template<>
  struct SampleTraits<8>
  {
    static Sound::Sample::Type ConvertChannel(int32_t v)
    {
      return Sound::Sample::MAX * v / 255;
    }
  };

  template<>
  struct SampleTraits<12>
  {
    static Sound::Sample::Type ConvertChannel(int32_t v)
    {
      return v * 16;
    }
  };

  template<>
  struct SampleTraits<16>
  {
    static Sound::Sample::Type ConvertChannel(int32_t v)
    {
      return static_cast<Sound::Sample::Type>(v);
    }
  };

  template<>
  struct SampleTraits<20>
  {
    static Sound::Sample::Type ConvertChannel(int32_t v)
    {
      return static_cast<Sound::Sample::Type>(v / 16);
    }
  };

  template<>
  struct SampleTraits<24>
  {
    static Sound::Sample::Type ConvertChannel(int32_t v)
    {
      return static_cast<Sound::Sample::Type>(v / 256);
    }
  };

  template<uint_t width>
  static Sound::Sample MakeMonoSample(int32_t val)
  {
    const auto converted = SampleTraits<width>::ConvertChannel(val);
    return Sound::Sample(converted, converted);
  }

  template<uint_t width>
  static Sound::Sample MakeStereoSample(int32_t left, int32_t right)
  {
    return Sound::Sample(SampleTraits<width>::ConvertChannel(left), SampleTraits<width>::ConvertChannel(right));
  }

  class FlacTune
  {
  public:
    explicit FlacTune(Model::Ptr data)
      : Data(std::move(data))
      , Decoder(::FLAC__stream_decoder_new(), &::FLAC__stream_decoder_delete)
      , Stream(*Data->Content)
    {
      static_assert(Sound::Sample::BITS == 16, "Incompatible sound sample bits count");
      static_assert(Sound::Sample::MID == 0, "Incompatible sound sample type");
      Require(Decoder.get());
      const auto status = ::FLAC__stream_decoder_init_stream(Decoder.get(), &DoRead, &DoSeek, &DoTell, &DoSize, &DoEof,
                                                             &DoWrite, nullptr, &DoError, this);
      if (status != FLAC__STREAM_DECODER_INIT_STATUS_OK)
      {
        throw MakeFormattedError(THIS_LINE, "Failed to create decoder. Error: {}",
                                 ::FLAC__StreamDecoderInitStatusString[status]);
      }
      Reset();
    }

    Sound::Chunk RenderFrame()
    {
      while (Chunk.empty())
      {
        Chunk.reserve(Data->MaxFrameSize);
        Require(::FLAC__stream_decoder_process_single(Decoder.get()));
        const auto state = ::FLAC__stream_decoder_get_state(Decoder.get());
        if (state == FLAC__STREAM_DECODER_SEARCH_FOR_FRAME_SYNC)
        {
          continue;
        }
        else
        {
          break;
        }
      }
      return std::move(Chunk);
    }

    void Reset()
    {
      Require(::FLAC__stream_decoder_reset(Decoder.get()));
      Require(::FLAC__stream_decoder_process_until_end_of_metadata(Decoder.get()));
      Chunk.clear();
    }

    void Seek(uint64_t sample)
    {
      if (!::FLAC__stream_decoder_seek_absolute(Decoder.get(), sample))
      {
        throw Error(THIS_LINE, "Failed to seek");
      }
    }

  private:
    static Binary::DataInputStream& GetStream(void* param)
    {
      return static_cast<FlacTune*>(param)->Stream;
    }

    static FLAC__StreamDecoderReadStatus DoRead(const FLAC__StreamDecoder* /*decoder*/, FLAC__byte buffer[],
                                                std::size_t* bytes, void* param)
    {
      if (*bytes > 0)
      {
        auto& stream = GetStream(param);
        *bytes = stream.Read(buffer, *bytes);
        return *bytes != 0 ? FLAC__STREAM_DECODER_READ_STATUS_CONTINUE : FLAC__STREAM_DECODER_READ_STATUS_END_OF_STREAM;
      }
      else
      {
        return FLAC__STREAM_DECODER_READ_STATUS_ABORT;
      }
    }

    static FLAC__StreamDecoderSeekStatus DoSeek(const FLAC__StreamDecoder* /*decoder*/, FLAC__uint64 position,
                                                void* param)
    {
      try
      {
        auto& stream = GetStream(param);
        stream.Seek(position);
        return FLAC__STREAM_DECODER_SEEK_STATUS_OK;
      }
      catch (const std::exception&)
      {
        return FLAC__STREAM_DECODER_SEEK_STATUS_ERROR;
      }
    }

    static FLAC__StreamDecoderTellStatus DoTell(const FLAC__StreamDecoder* /*decoder*/, FLAC__uint64* position,
                                                void* param)
    {
      auto& stream = GetStream(param);
      *position = stream.GetPosition();
      return FLAC__STREAM_DECODER_TELL_STATUS_OK;
    }

    static FLAC__StreamDecoderLengthStatus DoSize(const FLAC__StreamDecoder* /*decoder*/, FLAC__uint64* size,
                                                  void* param)
    {
      auto& self = *static_cast<FlacTune*>(param);
      *size = self.Data->Content->Size();
      return FLAC__STREAM_DECODER_LENGTH_STATUS_OK;
    }

    static FLAC__bool DoEof(const FLAC__StreamDecoder* /*decoder*/, void* param)
    {
      auto& stream = GetStream(param);
      return 0 == stream.GetRestSize();
    }

    static FLAC__StreamDecoderWriteStatus DoWrite(const FLAC__StreamDecoder* /*decoder*/, const FLAC__Frame* frame,
                                                  const FLAC__int32* const buffer[], void* param)
    {
      auto& self = *static_cast<FlacTune*>(param);
      if (frame->header.sample_rate != self.Data->Frequency)
      {
        return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
      }
      const auto samples = frame->header.blocksize;
      const auto bits = frame->header.bits_per_sample;
      const auto samplesBefore = self.Chunk.size();
      self.Chunk.resize(samplesBefore + samples);
      const auto target = self.Chunk.data() + samplesBefore;
      switch (frame->header.channels)
      {
      case 1:
        self.RenderMono(buffer[0], target, samples, bits);
        break;
      case 2:
        self.RenderStereo(buffer[0], buffer[1], target, samples, bits);
        break;
      default:
        return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
      }
      return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
    }

    static void DoError(const FLAC__StreamDecoder* /*decoder*/, FLAC__StreamDecoderErrorStatus /*status*/, void* param)
    {
      auto& self = *static_cast<FlacTune*>(param);
      self.Chunk.clear();
    }

    void RenderMono(const int32_t* mono, Sound::Sample* target, uint_t samples, uint_t width)
    {
      switch (width)
      {
      case 8:
        std::transform(mono, mono + samples, target, &MakeMonoSample<8>);
        break;
      case 12:
        std::transform(mono, mono + samples, target, &MakeMonoSample<12>);
        break;
      case 16:
        std::transform(mono, mono + samples, target, &MakeMonoSample<16>);
        break;
      case 20:
        std::transform(mono, mono + samples, target, &MakeMonoSample<20>);
        break;
      case 24:
        std::transform(mono, mono + samples, target, &MakeMonoSample<24>);
        break;
      default:
        break;
      }
    }

    void RenderStereo(const int32_t* left, const int32_t* right, Sound::Sample* target, uint_t samples, uint_t width)
    {
      switch (width)
      {
      case 8:
        std::transform(left, left + samples, right, target, &MakeStereoSample<8>);
        break;
      case 12:
        std::transform(left, left + samples, right, target, &MakeStereoSample<12>);
        break;
      case 16:
        std::transform(left, left + samples, right, target, &MakeStereoSample<16>);
        break;
      case 20:
        std::transform(left, left + samples, right, target, &MakeStereoSample<20>);
        break;
      case 24:
        std::transform(left, left + samples, right, target, &MakeStereoSample<24>);
        break;
      default:
        break;
      }
    }

  private:
    const Model::Ptr Data;
    using FlacPtr = std::shared_ptr<FLAC__StreamDecoder>;
    const FlacPtr Decoder;
    Binary::DataInputStream Stream;
    Sound::Chunk Chunk;
  };

  class Renderer : public Module::Renderer
  {
  public:
    Renderer(const Model::Ptr& data, Sound::Converter::Ptr target)
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
    FlacTune Tune;
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

  class DataBuilder : public Formats::Chiptune::Flac::Builder
  {
  public:
    explicit DataBuilder(PropertiesHelper& props)
      : Data(MakeRWPtr<Model>())
      , Properties(props)
      , Meta(props)
    {}

    Formats::Chiptune::MetaBuilder& GetMetaBuilder() override
    {
      return Meta;
    }

    void SetBlockSize(uint_t /*minimal*/, uint_t /*maximal*/) override {}
    void SetFrameSize(uint_t /*minimal*/, uint_t maximal) override
    {
      Data->MaxFrameSize = maximal;
    }

    void SetStreamParameters(uint_t sampleRate, uint_t channels, uint_t /*bitsPerSample*/) override
    {
      Require(channels <= 2);
      Data->Frequency = sampleRate;
    }

    void SetTotalSamples(uint64_t count) override
    {
      Data->TotalSamples = count;
    }

    void SetContent(Binary::Data::Ptr data)
    {
      Data->Content = std::move(data);
    }

    void AddFrame(std::size_t /*offset*/) override {}

    Model::Ptr GetResult()
    {
      if (Data->TotalSamples)
      {
        return Data;
      }
      else
      {
        return {};
      }
    }

  private:
    const Model::RWPtr Data;
    PropertiesHelper& Properties;
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
        if (const auto container = Formats::Chiptune::Flac::Parse(rawData, dataBuilder))
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
        Dbg("Failed to create FLAC: {}", e.what());
      }
      return {};
    }
  };
}  // namespace Module::Flac

namespace ZXTune
{
  void RegisterFLACPlugin(PlayerPluginsRegistrator& registrator)
  {
    const auto ID = "FLAC"_id;
    const uint_t CAPS = Capabilities::Module::Type::STREAM | Capabilities::Module::Device::DAC;

    auto decoder = Formats::Chiptune::CreateFLACDecoder();
    auto factory = MakePtr<Module::Flac::Factory>();
    auto plugin = CreatePlayerPlugin(ID, CAPS, std::move(decoder), std::move(factory));
    registrator.RegisterPlugin(std::move(plugin));
  }
}  // namespace ZXTune
