/**
 *
 * @file
 *
 * @brief  FFmpeg adapter implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include "core/plugins/players/music/ffmpeg_decoder.h"
// common includes
#include <contract.h>
#include <make_ptr.h>
#include <pointers.h>
// library includes
#include <math/numeric.h>
// std includes
#include <array>
// 3rdparty
extern "C"
{
#include "3rdparty/ffmpeg/libavcodec/avcodec.h"
}

extern AVCodec ff_atrac3_decoder;
extern AVCodec ff_atrac3p_decoder;
extern AVCodec ff_atrac9_decoder;

namespace Module::FFmpeg
{
  class DecoderImpl : public Decoder
  {
  public:
    using Ptr = std::unique_ptr<DecoderImpl>;

    explicit DecoderImpl(const AVCodec& codec)
      : Context(::avcodec_alloc_context3(&codec))
      , Frame(::av_frame_alloc())
    {}

    void SetBlockSize(uint_t blockSize)
    {
      Context->block_align = static_cast<int>(blockSize);
    }

    void SetChannels(uint_t channels)
    {
      Context->channels = static_cast<int>(channels);
    }

    void SetExtraData(Binary::View extradata)
    {
      Context->extradata = const_cast<uint8_t*>(extradata.As<uint8_t>());
      Context->extradata_size = static_cast<int>(extradata.Size());
    }

    void Init()
    {
      CheckError(::avcodec_open2(Context, Context->codec, nullptr));
    }

    ~DecoderImpl() override
    {
      ::av_packet_free(&Packet);
      ::av_frame_free(&Frame);
      Context->extradata = nullptr;
      ::avcodec_free_context(&Context);
    }

    void Decode(Binary::View frame, Sound::Chunk* output) override
    {
      AllocatePacket(frame);
      CheckError(::avcodec_send_packet(Context, Packet));
      for (;;)
      {
        const auto res = ::avcodec_receive_frame(Context, Frame);
        if (res == AVERROR(EAGAIN) || res == AVERROR(EOF))
        {
          break;
        }
        CheckError(res);
        if (output && Frame->nb_samples)
        {
          const auto prevSize = output->size();
          output->resize(prevSize + Frame->nb_samples);
          ConvertOutput(output->begin() + prevSize);
        }
      }
    }

  private:
    static void CheckError(int code)
    {
      Require(code == 0);
    }

    void AllocatePacket(Binary::View frame)
    {
      const auto frameSize = static_cast<int>(frame.Size());
      if (!Packet)
      {
        Packet = ::av_packet_alloc();
        CheckError(::av_new_packet(Packet, frameSize));
      }
      else if (Packet->size < frameSize)
      {
        CheckError(::av_grow_packet(Packet, frameSize - Packet->size));
      }
      else if (Packet->size > frameSize)
      {
        ::av_shrink_packet(Packet, frameSize);
      }
      std::memcpy(Packet->data, frame.As<uint8_t>(), frameSize);
    }

    void ConvertOutput(Sound::Sample* target) const
    {
      switch (Context->channels)
      {
      case 1:
        DecodeMono(target);
        break;
      case 2:
        DecodeStereo(target);
        break;
      default:
        Require(false);
        break;
      }
    }

    void DecodeMono(Sound::Sample* target) const
    {
      switch (Frame->format)
      {
      case AV_SAMPLE_FMT_S16P:
      case AV_SAMPLE_FMT_S16:
      {
        const auto* const begin = GetSamples<int16_t>(0);
        const auto* const end = begin + Frame->nb_samples;
        std::transform(begin, end, target, DecodeMonoSample<int16_t>);
      }
      break;
      case AV_SAMPLE_FMT_FLT:
      case AV_SAMPLE_FMT_FLTP:
      {
        const auto* const begin = GetSamples<float>(0);
        const auto* const end = begin + Frame->nb_samples;
        std::transform(begin, end, target, DecodeMonoSample<float>);
      }
      break;
      default:
        Require(false);
        break;
      }
    }

    void DecodeStereo(Sound::Sample* target) const
    {
      switch (Frame->format)
      {
      case AV_SAMPLE_FMT_S16P:
      {
        const auto* const begin1 = GetSamples<int16_t>(0);
        const auto* const begin2 = GetSamples<int16_t>(1);
        const auto* const end = begin1 + Frame->nb_samples;
        std::transform(begin1, end, begin2, target, DecodePlanarSample<int16_t>);
      }
      break;
      case AV_SAMPLE_FMT_S16:
      {
        const auto* const begin = GetSamples<std::array<int16_t, 2>>(0);
        const auto* const end = begin + Frame->nb_samples;
        std::transform(begin, end, target, DecodeStereoSample<int16_t>);
      }
      break;
      case AV_SAMPLE_FMT_FLTP:
      {
        const auto* const begin1 = GetSamples<float>(0);
        const auto* const begin2 = GetSamples<float>(1);
        const auto* const end = begin1 + Frame->nb_samples;
        std::transform(begin1, end, begin2, target, DecodePlanarSample<float>);
      }
      break;
      case AV_SAMPLE_FMT_FLT:
      {
        const auto* const begin = GetSamples<std::array<float, 2>>(0);
        const auto* const end = begin + Frame->nb_samples;
        std::transform(begin, end, target, DecodeStereoSample<float>);
      }
      break;
      default:
        Require(false);
        break;
      }
    }

    template<class T>
    const T* GetSamples(int plane) const
    {
      // Assume data is properly aligned!
      const void* const raw = Frame->data[plane];
      return static_cast<const T*>(raw);
    }

    inline static Sound::Sample::Type DecodeSample(int16_t s)
    {
      return s;
    }

    inline static Sound::Sample::Type DecodeSample(float s)
    {
      const auto wide = static_cast<Sound::Sample::WideType>(s * Sound::Sample::MAX);
      return Math::Clamp<Sound::Sample::WideType>(wide, Sound::Sample::MIN, Sound::Sample::MAX);
    }

    template<class T>
    inline static Sound::Sample DecodeMonoSample(T s)
    {
      const auto val = DecodeSample(s);
      return Sound::Sample(val, val);
    }

    template<class T>
    inline static Sound::Sample DecodePlanarSample(T l, T r)
    {
      return Sound::Sample(DecodeSample(l), DecodeSample(r));
    }

    template<class T>
    inline static Sound::Sample DecodeStereoSample(const std::array<T, 2>& s)
    {
      return DecodePlanarSample(s[0], s[1]);
    }

  private:
    AVCodecContext* Context;
    AVFrame* Frame;
    AVPacket* Packet = nullptr;
  };

  Decoder::Ptr CreateAtrac3Decoder(uint_t channels, uint_t blockSize, Binary::View config)
  {
    auto decoder = MakePtr<DecoderImpl>(ff_atrac3_decoder);
    decoder->SetChannels(channels);
    decoder->SetBlockSize(blockSize);
    decoder->SetExtraData(config);
    decoder->Init();
    return decoder;
  }

  Decoder::Ptr CreateAtrac3PlusDecoder(uint_t channels, uint_t blockSize)
  {
    auto decoder = MakePtr<DecoderImpl>(ff_atrac3p_decoder);
    decoder->SetChannels(channels);
    decoder->SetBlockSize(blockSize);
    decoder->Init();
    return decoder;
  }

  Decoder::Ptr CreateAtrac9Decoder(uint_t blockSize, Binary::View config)
  {
    auto decoder = MakePtr<DecoderImpl>(ff_atrac9_decoder);
    decoder->SetBlockSize(blockSize);
    decoder->SetExtraData(config);
    decoder->Init();
    return decoder;
  }
}  // namespace Module::FFmpeg
