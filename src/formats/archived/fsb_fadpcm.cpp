/**
 *
 * @file
 *
 * @brief  FSB FADPCM images support
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include "formats/archived/fsb_formats.h"
// common includes
#include <byteorder.h>
#include <contract.h>
#include <make_ptr.h>
// library includes
#include <formats/chiptune/music/wav.h>
// std includes
#include <algorithm>

namespace Formats::Archived::FSB
{
  namespace Fadpcm
  {
    struct SampleProperties
    {
      String Name;
      uint_t Frequency = 0;
      uint_t Channels = 0;
      uint_t Bits = 0;
      uint_t SamplesCount = 0;
      Binary::Container::Ptr Data;
    };

    class LazyContainer : public Binary::Container
    {
    public:
      explicit LazyContainer(SampleProperties&& props) noexcept
        : Properties(std::move(props))
      {}

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
        const uint_t chans = Properties.Channels;
        const uint_t blockSize = 140;
        const uint_t blockSizeSamples = 256;
        const uint32_t dummy = 0;
        builder->SetProperties(Format::EXTENDED, Properties.Frequency, chans, 4, blockSize * chans);
        builder->SetExtendedProperties(blockSizeSamples, (1 << chans) - 1, FADPCM, {&dummy, sizeof(dummy)});
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
        Require(format == Fmod::Format::FADPCM);
        Samples.resize(samplesCount);
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

      void AddMetaChunk(uint_t type, Binary::View chunk) override
      {
        if (type == Fmod::ChunkType::FREQUENCY && chunk.Size() >= sizeof(uint32_t))
        {
          Samples[CurSample].Frequency = ReadLE<uint32_t>(chunk.As<uint8_t>());
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
  }  // namespace Fadpcm

  FormatBuilder::Ptr CreateFadpcmBuilder()
  {
    return MakePtr<Fadpcm::Builder>();
  }
}  // namespace Formats::Archived::FSB
