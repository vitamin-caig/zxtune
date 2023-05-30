/**
 *
 * @file
 *
 * @brief  WAV ADPCM player code
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "core/plugins/players/music/wav_supp.h"
// common includes
#include <contract.h>
#include <make_ptr.h>
// library includes
#include <math/numeric.h>
// std includes
#include <array>

namespace Module::Wav
{
  static_assert(Sound::Sample::BITS == 16, "Incompatible sound sample bits count");
  static_assert(Sound::Sample::MID == 0, "Incompatible sound sample type");
  static_assert(Sound::Sample::CHANNELS == 2, "Incompatible sound sample channels count");

  using ConvertFunc = void (*)(const uint8_t*, std::size_t, Sound::Chunk*);

  Sound::Sample MakeSample(int16_t val)
  {
    return Sound::Sample(val, val);
  }

  int_t ReadS16(const uint8_t* data)
  {
    return static_cast<int16_t>(data[0] + 256 * data[1]);
  }

  namespace Adpcm
  {
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
      {}

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

    uint_t GetSamplesPerBlock(uint_t channels, std::size_t blockSize)
    {
      // return (blockSize - 7 * channels) * 2 / channels + 2;
      return blockSize * 2 / channels - 14 + 2;
    }

    void ConvertMono(const uint8_t* data, std::size_t blockSize, Sound::Chunk* chunk)
    {
      Require(*data < 7);
      Decoder dec(data[0]);
      dec.Delta = ReadS16(data + 1);
      dec.S1 = ReadS16(data + 3);
      dec.S2 = ReadS16(data + 5);
      chunk->push_back(MakeSample(dec.S2));
      chunk->push_back(MakeSample(dec.S1));
      for (data += 7, blockSize -= 7; blockSize != 0; ++data, --blockSize)
      {
        chunk->push_back(MakeSample(dec.Decode(*data >> 4)));
        chunk->push_back(MakeSample(dec.Decode(*data & 15)));
      }
    }

    void ConvertStereo(const uint8_t* data, std::size_t blockSize, Sound::Chunk* chunk)
    {
      Require(data[0] < 7 && data[1] < 7);
      Decoder left(data[0]);
      Decoder right(data[1]);
      left.Delta = ReadS16(data + 2);
      right.Delta = ReadS16(data + 4);
      left.S1 = ReadS16(data + 6);
      right.S1 = ReadS16(data + 8);
      left.S2 = ReadS16(data + 10);
      right.S2 = ReadS16(data + 12);
      chunk->push_back(Sound::Sample(left.S2, right.S2));
      chunk->push_back(Sound::Sample(left.S1, right.S1));
      for (data += 14, blockSize -= 14; blockSize != 0; ++data, --blockSize)
      {
        chunk->push_back(Sound::Sample(left.Decode(*data >> 4), right.Decode(*data & 15)));
      }
    }

    ConvertFunc GetConvertFunc(uint_t channels)
    {
      if (channels == 1)
      {
        return &ConvertMono;
      }
      else if (channels == 2)
      {
        return &ConvertStereo;
      }
      else
      {
        return nullptr;
      }
    }
  }  // namespace Adpcm

  // https://wiki.multimedia.cx/index.php/IMA_ADPCM
  // https://wiki.multimedia.cx/index.php/Microsoft_IMA_ADPCM
  namespace ImaAdpcm
  {
    static const std::array<int_t, 16> INDICES = {{-1, -1, -1, -1, 2, 4, 6, 8, -1, -1, -1, -1, 2, 4, 6, 8}};
    static const std::array<uint_t, 89> STEPS = {
        {7,    8,     9,     10,    11,    12,    13,    14,    16,    17,    19,    21,    23,    25,   28,
         31,   34,    37,    41,    45,    50,    55,    60,    66,    73,    80,    88,    97,    107,  118,
         130,  143,   157,   173,   190,   209,   230,   253,   279,   307,   337,   371,   408,   449,  494,
         544,  598,   658,   724,   796,   876,   963,   1060,  1166,  1282,  1411,  1552,  1707,  1878, 2066,
         2272, 2499,  2749,  3024,  3327,  3660,  4026,  4428,  4871,  5358,  5894,  6484,  7132,  7845, 8630,
         9493, 10442, 11487, 12635, 13899, 15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794, 32767}};

    struct Decoder
    {
      int_t Predictor;
      int_t Index;

      int_t Decode(uint_t nibble)
      {
        const auto step = STEPS[Index];
        int_t diff = step >> 3;
        if (nibble & 1)
        {
          diff += step >> 2;
        }
        if (nibble & 2)
        {
          diff += step >> 1;
        }
        if (nibble & 4)
        {
          diff += step;
        }
        if (nibble & 8)
        {
          diff = -diff;
        }
        Predictor = Math::Clamp(Predictor + diff, -32768, 32767);
        Index = Math::Clamp<int_t>(Index + INDICES[nibble], 0, STEPS.size() - 1);
        return Predictor;
      }
    };

    uint_t GetSamplesPerBlock(uint_t channels, std::size_t blockSize)
    {
      // return (blockSize - 4 * channels) * 2 / channels + 1;
      return blockSize * 2 / channels - 8 + 1;
    }

    void ConvertMono(const uint8_t* data, std::size_t blockSize, Sound::Chunk* chunk)
    {
      Require(data[2] < STEPS.size());
      Decoder dec;
      dec.Predictor = ReadS16(data);
      dec.Index = data[2];
      chunk->push_back(MakeSample(dec.Predictor));
      for (data += 4, blockSize -= 4; blockSize != 0; ++data, --blockSize)
      {
        chunk->push_back(MakeSample(dec.Decode(*data & 15)));
        chunk->push_back(MakeSample(dec.Decode(*data >> 4)));
      }
    }

    void ConvertStereo(const uint8_t* data, std::size_t blockSize, Sound::Chunk* chunk)
    {
      Require(data[2] < STEPS.size() && data[6] < STEPS.size());
      Decoder left;
      Decoder right;
      left.Predictor = ReadS16(data);
      left.Index = data[2];
      right.Predictor = ReadS16(data + 4);
      right.Index = data[6];
      chunk->push_back(Sound::Sample(left.Predictor, right.Predictor));
      for (data += 8, blockSize -= 8; blockSize >= 8; blockSize -= 8)
      {
        for (uint_t byte = 0; byte < 4; ++byte, ++data)
        {
          chunk->push_back(Sound::Sample(left.Decode(data[0] & 15), right.Decode(data[4] & 15)));
          chunk->push_back(Sound::Sample(left.Decode(data[0] >> 4), right.Decode(data[4] >> 4)));
        }
        data += 4;
      }
    }

    ConvertFunc GetConvertFunc(uint_t channels)
    {
      if (channels == 1)
      {
        return &ConvertMono;
      }
      else if (channels == 2)
      {
        return &ConvertStereo;
      }
      else
      {
        return nullptr;
      }
    }
  }  // namespace ImaAdpcm

  class AdpcmModel : public BlockingModel
  {
  public:
    AdpcmModel(Properties props, ConvertFunc convert)
      : BlockingModel(std::move(props))
      , Convert(convert)
    {}

    Sound::Chunk RenderNextFrame() override
    {
      Sound::Chunk chunk;
      if (const auto* start = Stream.PeekRawData(Props.BlockSize))
      {
        chunk.reserve(Props.BlockSizeSamples);
        Convert(start, Props.BlockSize, &chunk);
        Stream.Skip(Props.BlockSize);
      }
      return chunk;
    }

  private:
    const ConvertFunc Convert;
  };

  Model::Ptr CreateAdpcmModel(Properties props)
  {
    Require(props.Bits == 4);
    Require(props.BlockSize > 32);  // TODO
    const auto func = Adpcm::GetConvertFunc(props.Channels);
    Require(func);
    props.BlockSizeSamples = Adpcm::GetSamplesPerBlock(props.Channels, props.BlockSize);
    return MakePtr<AdpcmModel>(std::move(props), func);
  }

  Model::Ptr CreateImaAdpcmModel(Properties props)
  {
    Require(props.Bits == 4);
    Require(props.BlockSize > 32);  // TODO
    const auto func = ImaAdpcm::GetConvertFunc(props.Channels);
    Require(func);
    props.BlockSizeSamples = ImaAdpcm::GetSamplesPerBlock(props.Channels, props.BlockSize);
    return MakePtr<AdpcmModel>(std::move(props), func);
  }
}  // namespace Module::Wav
