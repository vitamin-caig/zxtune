/**
* 
* @file
*
* @brief  WAV ADPCM player code
*
* @author vitamin.caig@gmail.com
*
**/

#include "wav_supp.h"
//common includes
#include <contract.h>
#include <make_ptr.h>
//library includes
#include <math/numeric.h>
#include <sound/chunk_builder.h>

namespace Module
{
namespace Wav
{
  namespace Adpcm
  {
    static_assert(Sound::Sample::BITS == 16, "Incompatible sound sample bits count");
    static_assert(Sound::Sample::MID == 0, "Incompatible sound sample type");
    static_assert(Sound::Sample::CHANNELS == 2, "Incompatible sound sample channels count");
    
    static const int_t ADAPTATION[] = {230, 230, 230, 230, 307, 409, 512, 614, 768, 614, 512, 409, 307, 230, 230, 230};
    static const int_t COEFF1[] = {256, 512, 0, 192, 240, 460, 392};
    static const int_t COEFF2[] = {0, -256, 0, 64, 0, -208, -232};
    
    int_t ToSigned(int_t nibble)
    {
      return nibble >= 8 ? nibble - 16 : nibble;
    }
    
    struct Decoder
    {
      explicit Decoder(uint_t predictor)
        : C1(COEFF1[predictor])
        , C2(COEFF2[predictor])
      {
      }
      
      int_t Decode(uint_t nibble)
      {
        const auto predictor = ((S1 * C1) + (S2 * C2)) / 256 + ToSigned(nibble) * Delta;
        S2 = S1;
        S1 = Math::Clamp(predictor, -32768, 32767);
        Delta = std::max<int_t>((ADAPTATION[nibble] * Delta) / 256, 16);
        return S1;
      }
      
      const int_t C1;
      const int_t C2;
      int_t Delta;
      int_t S1;
      int_t S2;
    };
    
    Sound::Sample MakeSample(int16_t val)
    {
      return Sound::Sample(val, val);
    }
    
    int_t ReadS16(const uint8_t* data)
    {
      return static_cast<int16_t>(data[0] + 256 * data[1]);
    }
    
    using ConvertFunc = Sound::Chunk(*)(const uint8_t*, std::size_t);
    
    Sound::Chunk ConvertMono(const uint8_t* data, std::size_t blockSize)
    {
      Sound::ChunkBuilder builder;
      builder.Reserve((blockSize - 7) * 2 + 2);
      Require(*data < 7);
      Decoder dec(data[0]);
      dec.Delta = ReadS16(data + 1);
      dec.S1 = ReadS16(data + 3);
      dec.S2 = ReadS16(data + 5);
      builder.Add(MakeSample(dec.S2));
      builder.Add(MakeSample(dec.S1));
      for (data += 7, blockSize -= 7; blockSize != 0; ++data, --blockSize)
      {
        builder.Add(MakeSample(dec.Decode(*data >> 4)));
        builder.Add(MakeSample(dec.Decode(*data & 15)));
      }
      return builder.CaptureResult();
    }
    
    Sound::Chunk ConvertStereo(const uint8_t* data, std::size_t blockSize)
    {
      Sound::ChunkBuilder builder;
      builder.Reserve((blockSize - 14) + 2);
      Require(data[0] < 7 && data[1] < 7);
      Decoder left(data[0]);
      Decoder right(data[1]);
      left.Delta = ReadS16(data + 2);
      right.Delta = ReadS16(data + 4);
      left.S1 = ReadS16(data + 6);
      right.S1 = ReadS16(data + 8);
      left.S2 = ReadS16(data + 10);
      right.S2 = ReadS16(data + 12);
      builder.Add(Sound::Sample(left.S2, right.S2));
      builder.Add(Sound::Sample(left.S1, right.S1));
      for (data += 14, blockSize -= 14; blockSize != 0; ++data, --blockSize)
      {
        builder.Add(Sound::Sample(left.Decode(*data >> 4), right.Decode(*data & 15)));
      }
      return builder.CaptureResult();
    }
  }
  
  Adpcm::ConvertFunc GetConvertFunc(uint_t channels)
  {
    if (channels == 1)
    {
      return &Adpcm::ConvertMono;
    }
    else if (channels == 2)
    {
      return &Adpcm::ConvertStereo;
    }
    else
    {
      return nullptr;
    }
  }

  class AdpcmModel : public Model
  {
  public:
    AdpcmModel(Adpcm::ConvertFunc convert, const Properties& props)
      : Convert(convert)
      , Data(props.Data)
      , Frequency(props.Frequency)
      , BlockSize(props.BlockSize)
      , TotalBlocks(Data->Size() / BlockSize)
    {
    }

    uint_t GetFrequency() const override
    {
      return Frequency;
    }
    
    uint_t GetFramesCount() const override
    {
      return TotalBlocks;
    }
    
    Sound::Chunk RenderFrame(uint_t idx) const override
    {
      const auto start = static_cast<const uint8_t*>(Data->Start()) + BlockSize * idx;
      return Convert(start, BlockSize);
    }
    
  private:
    const Adpcm::ConvertFunc Convert;
    const Binary::Data::Ptr Data;
    const uint_t Frequency;
    const std::size_t BlockSize;
    const uint_t TotalBlocks;
  };
  
  Model::Ptr CreateAdpcmModel(const Properties& props)
  {
    Require(props.Bits == 4);
    Require(props.BlockSize > 32);//TODO
    const auto func = GetConvertFunc(props.Channels);
    Require(func);
    return MakePtr<AdpcmModel>(func, props);
  }
}
}
