/**
* 
* @file
*
* @brief  ATRAC9 player code
*
* @author vitamin.caig@gmail.com
*
**/

#include "core/plugins/players/music/wav_supp.h"
//common includes
#include <contract.h>
#include <make_ptr.h>
#include <pointers.h>
//library includes
#include <binary/input_stream.h>
#include <sound/chunk_builder.h>
//3rdparty includes
#include <3rdparty/atrac9/C/src/libatrac9.h>
#include <3rdparty/atrac9/C/src/error_codes.h>

namespace Module
{
namespace Wav
{
  class Atrac9Model : public Model
  {
  public:
    Atrac9Model(const Properties& props, Binary::View config)
      : Handle( ::Atrac9GetHandle(), &::Atrac9ReleaseHandle)
      , Data(props.Data)
      , TotalSamples(props.SamplesCountHint)
    {
      Require(config.Size() >= ATRAC9_CONFIG_DATA_SIZE);
      CheckResult( ::Atrac9InitDecoder(Handle.get(), static_cast<const uint8_t*>(config.Start())));
      CheckResult( ::Atrac9GetCodecInfo(Handle.get(), &Info));
    }

    uint_t GetFrequency() const override
    {
      return Info.samplingRate;
    }
    
    uint_t GetFramesCount() const override
    {
      const auto superFrameSize = Info.frameSamples * Info.framesInSuperframe;
      return (TotalSamples + superFrameSize - 1) / superFrameSize;
    }
    
    uint_t GetSamplesPerFrame() const override
    {
      return Info.frameSamples * Info.framesInSuperframe;
    }
    
    Sound::Chunk RenderFrame(uint_t idx) const override
    {
      static const std::size_t MIN_FRAME_SIZE = 10;
      const auto offset = idx * Info.superframeSize;
      auto* src = static_cast<const uint8_t*>(Data->Start()) + offset;
      Sound::ChunkBuilder builder;
      builder.Reserve(Info.frameSamples * Info.framesInSuperframe);
      std::size_t restData = Data->Size() - offset;
      for (int idx = 0; idx < Info.framesInSuperframe && restData > MIN_FRAME_SIZE; ++idx)
      {
        int usedData = 0;
        auto* dst = builder.Allocate(Info.frameSamples);
        CheckResult( ::Atrac9Decode(Handle.get(), src, safe_ptr_cast<short*>(dst), &usedData));
        ConvertOutput(dst);
        src += usedData;
        restData -= usedData;
      }
      return builder.CaptureResult();
    }
  private:
    static void CheckResult(int res)
    {
      Require(res == ERR_SUCCESS);
    }    

    void ConvertOutput(Sound::Sample* buf) const
    {
      if (Info.channels != 1)
      {
        return;
      }
      const auto* monoBuf = safe_ptr_cast<const short*>(buf);
      for (std::size_t idx = Info.frameSamples; idx != 0; --idx)
      {
        const auto mono = monoBuf[idx - 1];
        buf[idx - 1] = Sound::Sample(mono, mono);
      }
    }
  private:
    const std::shared_ptr<void> Handle;
    const Binary::Data::Ptr Data;
    const std::size_t TotalSamples;
    ::Atrac9CodecInfo Info;
  };

  Model::Ptr CreateAtrac9Model(const Properties& props, Binary::View extraData)
  {
    Require(props.Channels <= Sound::Sample::CHANNELS);
    Require(props.SamplesCountHint != 0);
    Binary::DataInputStream extra(extraData);
    const auto vers = extra.ReadLE<uint32_t>();
    Require(vers < 2);
    return MakePtr<Atrac9Model>(props, extra.ReadRestData());
  }
}
}
