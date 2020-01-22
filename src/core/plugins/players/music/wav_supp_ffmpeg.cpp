/**
* 
* @file
*
* @brief  FFmpeg-based model
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "core/plugins/players/music/wav_supp.h"
#include "core/plugins/players/music/ffmpeg_decoder.h"
//common includes
#include <contract.h>
#include <make_ptr.h>

namespace Module
{
namespace Wav
{
  class FFmpegModel : public Model
  {
  public:
    FFmpegModel(Properties props, FFmpeg::Decoder::Ptr decoder)
      : Props(std::move(props))
      , Decoder(std::move(decoder))
    {
    }

    uint_t GetFrequency() const override
    {
      return Props.Frequency;
    }
    
    uint_t GetFramesCount() const override
    {
      return (Props.SamplesCountHint + Props.BlockSizeSamples - 1) / Props.BlockSizeSamples;
    }
    
    uint_t GetSamplesPerFrame() const override
    {
      return Props.BlockSizeSamples;
    }
    
    Sound::Chunk RenderFrame(uint_t idx) const override
    {
      const auto data = Binary::View(*Props.Data).SubView(idx * Props.BlockSize, Props.BlockSize);
      Sound::Chunk result;
      result.reserve(Props.BlockSizeSamples);
      Decoder->Decode(data, &result);
      return result;
    }
  private:
    Properties Props;
    const FFmpeg::Decoder::Ptr Decoder;
  };

  Model::Ptr CreateAtrac3Model(const Properties& props, Binary::View extraData)
  {
    Require(props.Channels <= Sound::Sample::CHANNELS);
    Require(props.SamplesCountHint != 0);
    Require(props.BlockSize != 0);
    auto propsCopy = props;
    propsCopy.BlockSizeSamples = 1024;
    auto decoder = FFmpeg::CreateAtrac3Decoder(props.Channels, props.BlockSize, extraData);
    return MakePtr<FFmpegModel>(std::move(propsCopy), std::move(decoder));
  }

  Model::Ptr CreateAtrac3PlusModel(const Properties& props)
  {
    Require(props.Channels <= Sound::Sample::CHANNELS);
    Require(props.SamplesCountHint != 0);
    Require(props.BlockSizeSamples != 0);
    Require(props.BlockSize != 0);
    auto decoder = FFmpeg::CreateAtrac3PlusDecoder(props.Channels, props.BlockSize);
    return MakePtr<FFmpegModel>(props, std::move(decoder));
  }
}
}
