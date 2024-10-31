/**
 *
 * @file
 *
 * @brief  FFmpeg-based model
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "core/plugins/players/music/ffmpeg_decoder.h"
#include "core/plugins/players/music/wav_supp.h"

#include "contract.h"
#include "make_ptr.h"

namespace Module::Wav
{
  class FFmpegModel : public BlockingModel
  {
  public:
    FFmpegModel(Properties props, FFmpeg::Decoder::Ptr decoder)
      : BlockingModel(std::move(props))
      , Decoder(std::move(decoder))
    {}

    Sound::Chunk RenderNextFrame() override
    {
      Sound::Chunk result;
      if (const auto* data = Stream.PeekRawData(Props.BlockSize))
      {
        result.reserve(Props.BlockSizeSamples);
        Decoder->Decode({data, Props.BlockSize}, &result);
        Stream.Skip(Props.BlockSize);
      }
      return result;
    }

  private:
    const FFmpeg::Decoder::Ptr Decoder;
  };

  Model::Ptr CreateAtrac3Model(Properties props, Binary::View extraData)
  {
    Require(props.Channels <= Sound::Sample::CHANNELS);
    Require(props.SamplesCountHint != 0);
    props.BlockSizeSamples = 1024;
    auto decoder = FFmpeg::CreateAtrac3Decoder(props.Channels, props.BlockSize, extraData);
    return MakePtr<FFmpegModel>(std::move(props), std::move(decoder));
  }

  Model::Ptr CreateAtrac3PlusModel(Properties props)
  {
    Require(props.Channels <= Sound::Sample::CHANNELS);
    Require(props.SamplesCountHint != 0);
    auto decoder = FFmpeg::CreateAtrac3PlusDecoder(props.Channels, props.BlockSize);
    return MakePtr<FFmpegModel>(std::move(props), std::move(decoder));
  }

  Model::Ptr CreateAtrac9Model(Properties props, Binary::View extraData)
  {
    Require(props.Channels <= Sound::Sample::CHANNELS);
    Require(props.SamplesCountHint != 0);
    Require(props.BlockSizeSamples != 0);
    Require(props.BlockSize != 0);
    auto decoder = FFmpeg::CreateAtrac9Decoder(props.BlockSize, extraData);
    return MakePtr<FFmpegModel>(std::move(props), std::move(decoder));
  }
}  // namespace Module::Wav
