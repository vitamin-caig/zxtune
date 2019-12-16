/**
* 
* @file
*
* @brief  FSB ATRAC9 images support
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "formats/archived/fsb_formats.h"
//library includes
#include <formats/chiptune/music/wav.h>
//common includes
#include <contract.h>
#include <make_ptr.h>

namespace Formats
{
namespace Archived
{
namespace FSB
{
  namespace Atrac9
  {
    class ConfigBlock
    {
    public:
      ConfigBlock()
      {
        // Set version to 1
        Data[0] = 1;
      }

      void Set(Binary::View chunk)
      {
        const std::size_t CONFIG_SIZE = 4;
        const auto* begin = chunk.As<uint8_t>();
        const auto* end = begin + chunk.Size();
        const auto* sign = std::find(begin, end, 0xfe);
        Require(sign + CONFIG_SIZE <= end);
        std::memcpy(&Data[4], sign, CONFIG_SIZE);
        // check data here
        Require(GetChannelsCount() <= 2);
        //verification bit
        Require((Data[5] & 1) == 0);
      }

      uint_t GetSamplerate() const
      {
        static const uint_t SAMPLERATES[16] =
        {
          11025, 12000, 16000, 22050, 24000, 32000, 44100, 48000,
          44100, 48000, 64000, 88200, 96000, 128000, 176400, 192000
        };
        return SAMPLERATES[Data[5] >> 4];
      }

      uint_t GetChannelsCount() const
      {
        switch ((Data[5] >> 1) & 7)
        {
        case 0://mono
          return 1;
        case 1://dual mono
        case 2://stereo
          return 2;
        default:
          return 6;
        }
      }

      uint_t GetBlockSize() const
      {
        const auto frameBytes = 1 + ((uint_t(Data[6]) << 3) | (Data[7] >> 5));
        const auto superframeIndex = (Data[7] >> 3) & 3;
        return frameBytes << superframeIndex;
      }

      Binary::View Get() const
      {
        //marker
        Require(Data[4] == 0xfe);
        return Data;
      }
   private:
      // Standard extra part for ATRAC9 streams
      std::array<uint8_t, 12> Data;
    };

    struct SampleProperties
    {
      String Name;
      ConfigBlock Config;
      uint_t SamplesCount = 0;
      Binary::Container::Ptr Data;
    };
    
    class LazyContainer : public Binary::Container
    {
    public:
      explicit LazyContainer(SampleProperties&& props) noexcept
        : Properties(std::move(props))
      {
      }

      const void* Start() const override
      {
        if (!Delegate)
        {
          Build();
        }
        return Delegate->Start();
      }

      std::size_t Size() const override
      {
        if (!Delegate)
        {
          Build();
        }
        return Delegate->Size();
      }
      
      Ptr GetSubcontainer(std::size_t offset, std::size_t size) const override
      {
        if (!Delegate)
        {
          Build();
        }
        return Delegate->GetSubcontainer(offset, size);
      }
    private:
      void Build() const
      {
        using namespace Chiptune::Wav;
        const auto builder = CreateDumpBuilder();
        const auto freq = Properties.Config.GetSamplerate();
        const auto chans = Properties.Config.GetChannelsCount();
        const auto bits = 0;
        const auto blockSize = Properties.Config.GetBlockSize();
        builder->SetProperties(Format::EXTENDED, freq, chans, bits, blockSize);
        builder->SetExtendedProperties(blockSize, (1 << chans) - 1, ATRAC9, Properties.Config.Get());
        builder->SetSamplesCountHint(Properties.SamplesCount);
        builder->SetSamplesData(std::move(Properties.Data));
        Delegate = builder->GetDump();
      }
    private:
      mutable SampleProperties Properties;
      mutable Ptr Delegate;
    };
    
    class Builder : public FormatBuilder
    {
    public:
      void Setup(uint_t samplesCount, uint_t format) override
      {
        Require(format == Fmod::Format::AT9);
        Samples.resize(samplesCount);
      }
      
      void StartSample(uint_t idx) override
      {
        CurSample = idx;
      }
      
      void SetFrequency(uint_t /*frequency*/) override
      {
      }
      
      void SetChannels(uint_t /*channels*/) override
      {
      }
      
      void SetName(String name) override
      {
        Samples[CurSample].Name = std::move(name);
      }
      
      void AddMetaChunk(uint_t type, Binary::View chunk) override
      {
        if (type == Fmod::ChunkType::ATRAC9DATA)
        {
          Samples[CurSample].Config.Set(chunk);
        }
      }
      
      void SetData(uint_t samplesCount, Binary::Container::Ptr blob) override
      {
        auto& dst = Samples[CurSample];
        Require(samplesCount != 0);
        dst.SamplesCount = samplesCount;
        dst.Data = std::move(blob);
      }
      
      NamedDataMap CaptureResult() override
      {
        NamedDataMap result;
        for (auto& smp : Samples)
        {
          if (smp.Data)
          {
            auto name = std::move(smp.Name);
            result.emplace(name, MakePtr<LazyContainer>(std::move(smp)));
          }
        }
        return result;
      }
    private:
      uint_t CurSample = 0;
      std::vector<SampleProperties> Samples;
    };
  }

  FormatBuilder::Ptr CreateAtrac9Builder()
  {
    return MakePtr<Atrac9::Builder>();
  }
}
}
}
