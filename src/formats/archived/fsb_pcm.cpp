/**
* 
* @file
*
* @brief  FSB PCM images support
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "fsb_formats.h"
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
    struct SampleProperties
    {
      SampleProperties(uint_t format)
      {
        switch (format)
        {
        case Fmod::Format::PCM8:
          Bits = 8;
          break;
        case Fmod::Format::PCM16:
          Bits = 16;
          break;
        case Fmod::Format::PCM24:
          Bits = 24;
          break;
        case Fmod::Format::PCM32:
          Bits = 32;
          break;
        case Fmod::Format::PCMFLOAT:
          Bits = 32;
          IsFloat = true;
          break;
        case Fmod::Format::IMAADPCM:
          Bits = 4;
          IsAdpcm = true;
          break;
        default:
          Require(false);
        }
      }
      
      String Name;
      uint_t Frequency = 0;
      uint_t Channels = 0;
      uint_t Bits = 0;
      bool IsFloat = false;
      bool IsAdpcm = false;
      uint_t SamplesCount = 0;
      Binary::Container::Ptr Data;
    };
    
    class LazyWavFile : public Binary::Container
    {
    public:
      explicit LazyWavFile(SampleProperties&& props)
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
        using namespace Formats::Chiptune::Wav;
        const auto builder = CreateDumpBuilder();
        const auto format = Properties.IsFloat ? Format::IEEE_FLOAT : (Properties.IsAdpcm ? Format::IMA_ADPCM : Format::PCM);
        const auto blockSize = Properties.IsAdpcm ? 36 * Properties.Channels : (Properties.Channels * Properties.Bits + 7) / 8;
        builder->SetProperties(format, Properties.Frequency, Properties.Channels, Properties.Bits, blockSize);
        if (Properties.SamplesCount)
        {
          builder->SetSamplesCountHint(Properties.SamplesCount);
        }
        builder->SetSamplesData(std::move(Properties.Data));
        Delegate = builder->GetDump();
      }
    private:
      mutable SampleProperties Properties;
      mutable Ptr Delegate;
    };
    
    class PcmBuilder : public FormatBuilder
    {
    public:
      void Setup(uint_t samplesCount, uint_t format) override
      {
        Samples.resize(samplesCount, SampleProperties(format));
      }
      
      void StartSample(uint_t idx) override
      {
        CurSample = idx;
      }
      
      void SetFrequency(uint_t frequency) override
      {
        Samples[CurSample].Frequency = frequency;
      }
      
      void SetChannels(uint_t channels) override
      {
        Samples[CurSample].Channels = channels;
      }
      
      void SetName(String name) override
      {
        Samples[CurSample].Name = std::move(name);
      }
      
      void AddMetaChunk(uint_t /*type*/, const Binary::Data& /*chunk*/) override
      {
      }
      
      void SetData(uint_t samplesCount, Binary::Container::Ptr blob) override
      {
        auto& dst = Samples[CurSample];
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
            result.emplace(name, MakePtr<LazyWavFile>(std::move(smp)));
          }
        }
        return result;
      }
    private:
      uint_t CurSample = 0;
      std::vector<SampleProperties> Samples;
    };

    FormatBuilder::Ptr CreatePcmBuilder()
    {
      return MakePtr<PcmBuilder>();
    }
  }
}
}
