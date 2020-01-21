/**
*
* @file
*
* @brief  FFmpeg adapter implementation
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "core/plugins/players/music/ffmpeg_decoder.h"
//common includes
#include <contract.h>
#include <pointers.h>
#include <make_ptr.h>
//3rdparty
extern "C" {
#include "3rdparty/ffmpeg/libavcodec/avcodec.h"
}

namespace Module
{
namespace FFmpeg
{
  class DecoderImpl : public Decoder
  {
  public:
    DecoderImpl(const AVCodec& codec, Binary::View extradata = Binary::View(nullptr, 0), uint_t blockAlign = 0)
      : Context(::avcodec_alloc_context3(&codec))
      , Frame(::av_frame_alloc())
    {
      Context->block_align = static_cast<int>(blockAlign);
      Context->extradata = const_cast<uint8_t*>(extradata.As<uint8_t>());
      Context->extradata_size = static_cast<int>(extradata.Size());
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
      switch(Frame->format)
      {
      case AV_SAMPLE_FMT_S16P:
      case AV_SAMPLE_FMT_S16:
        {
          const auto begin = safe_ptr_cast<const int16_t*>(Frame->data[0]);
          const auto end = begin + Frame->nb_samples;
          std::transform(begin, end, target, DecodeMonoSample<int16_t>);
        }
        break;
      case AV_SAMPLE_FMT_FLT:
      case AV_SAMPLE_FMT_FLTP:
        {
          const auto begin = safe_ptr_cast<const float*>(Frame->data[0]);
          const auto end = begin + Frame->nb_samples;
          std::transform(begin, end, target, DecodeMonoSample<int16_t>);
        }
        break;
      default:
        Require(false);
        break;
      }
    }

    void DecodeStereo(Sound::Sample* target) const
    {
      switch(Frame->format)
      {
      case AV_SAMPLE_FMT_S16P:
        {
          const auto begin1 = safe_ptr_cast<const int16_t*>(Frame->data[0]);
          const auto begin2 = safe_ptr_cast<const int16_t*>(Frame->data[1]);
          const auto end = begin1 + Frame->nb_samples;
          std::transform(begin1, end, begin2, target, DecodePlanarSample<int16_t>);
        }
        break;
      case AV_SAMPLE_FMT_S16:
        {
          const auto begin = safe_ptr_cast<const std::array<int16_t, 2>*>(Frame->data[0]);
          const auto end = begin + Frame->nb_samples;
          std::transform(begin, end, target, DecodeStereoSample<int16_t>);
        }
        break;
      case AV_SAMPLE_FMT_FLTP:
        {
          const auto begin1 = safe_ptr_cast<const float*>(Frame->data[0]);
          const auto begin2 = safe_ptr_cast<const float*>(Frame->data[1]);
          const auto end = begin1 + Frame->nb_samples;
          std::transform(begin1, end, begin2, target, DecodePlanarSample<float>);
        }
        break;
      case AV_SAMPLE_FMT_FLT:
        {
          const auto begin = safe_ptr_cast<const std::array<float, 2>*>(Frame->data[0]);
          const auto end = begin + Frame->nb_samples;
          std::transform(begin, end, target, DecodeStereoSample<float>);
        }
        break;
      default:
        Require(false);
        break;
      }
    }

    inline static Sound::Sample::Type DecodeSample(int16_t s)
    {
      return s;
    }

    inline static Sound::Sample::Type DecodeSample(float s)
    {
      return static_cast<Sound::Sample::Type>(s * Sound::Sample::MAX);
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
}
}
