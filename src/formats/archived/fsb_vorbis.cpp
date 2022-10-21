/**
 *
 * @file
 *
 * @brief  FSB Vorbis images support
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include "formats/archived/fsb_formats.h"
// library includes
#include <binary/input_stream.h>
#include <debug/log.h>
#include <formats/chiptune/music/oggvorbis.h>
// common includes
#include <contract.h>
#include <make_ptr.h>

namespace Formats::Archived::FSB
{
  namespace Vorbis
  {
    const Debug::Stream Dbg("Formats::Archived::FSB::Vorbis");

    struct SetupSection
    {
      const uint32_t Crc32;
      const StringView Data;
    };

    const SetupSection* GetSetup(uint32_t crc)
    {
      static const SetupSection SECTIONS[] = {
#include "formats/archived/fsb_vorbis_headers.inc"
      };
      // Require(std::is_sorted(SECTIONS, std::end(SECTIONS),
      //                       [](const SetupSection& lh, const SetupSection& rh) { return lh.Crc32 < rh.Crc32; }));
      const auto it = std::lower_bound(SECTIONS, std::end(SECTIONS), crc,
                                       [](const SetupSection& s, uint32_t crc) { return s.Crc32 < crc; });
      return it != std::end(SECTIONS) && it->Crc32 == crc ? it : nullptr;
    }

    struct SampleProperties
    {
      String Name;
      uint_t Frequency = 0;
      uint_t Channels = 0;
      uint_t SamplesCount = 0;
      const SetupSection* Setup = nullptr;
      Binary::Container::Ptr Data;

      struct LookupEntry
      {
        LookupEntry() = default;
        LookupEntry(std::size_t offset, uint_t position) noexcept
          : Offset(offset)
          , Position(position)
        {}

        // From start of Data in bytes
        std::size_t Offset = 0;
        // Up to SamplesCount
        uint_t Position = 0;

        bool Before(const LookupEntry& rh) const
        {
          return Offset < rh.Offset && Position <= rh.Position;
        }
      };
      std::vector<LookupEntry> Lookup;
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
        const auto vorbisDataSize = Properties.Data->Size();
        // use 12.5% overhead
        const auto builder = Chiptune::OggVorbis::CreateDumpBuilder(vorbisDataSize + vorbisDataSize / 8);
        builder->SetStreamId(Properties.Setup->Crc32);
        builder->SetProperties(Properties.Channels, Properties.Frequency, 256, 2048);
        builder->SetSetup(Binary::View(Properties.Setup->Data.data(), Properties.Setup->Data.size()));
        DumpFrames(*builder);
        Properties.Data = {};
        Properties.Lookup = {};
        Delegate = builder->GetDump();
      }

      void DumpFrames(Chiptune::OggVorbis::Builder& builder) const
      {
        auto lookupIt = Properties.Lookup.begin();
        SampleProperties::LookupEntry cursor, delta;
        for (Binary::InputStream stream(*Properties.Data); stream.GetRestSize() > sizeof(uint16_t);)
        {
          const auto offset = stream.GetPosition();
          while (offset >= lookupIt->Offset)
          {
            cursor = *lookupIt;
            ++lookupIt;
            delta = {lookupIt->Offset - cursor.Offset, lookupIt->Position - cursor.Position};
          }
          const std::size_t size = stream.Read<le_uint16_t>();
          if (const auto data = size ? stream.PeekRawData(size) : nullptr)
          {
            const auto totalSize = size + sizeof(uint16_t);
            const auto samples = totalSize * delta.Position / delta.Offset;
            builder.AddFrame(0, samples, Binary::View(data, size));
            stream.Skip(size);
            delta.Offset -= totalSize;
            delta.Position -= samples;
          }
          else
          {
            break;
          }
        }
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
        Require(format == Fmod::Format::VORBIS);
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
        if (type != Fmod::ChunkType::VORBISDATA)
        {
          return;
        }
        Binary::DataInputStream stream(chunk);
        auto& dst = Samples[CurSample];
        const auto crc = stream.Read<le_uint32_t>();
        dst.Setup = GetSetup(crc);
        dst.Lookup.emplace_back(0, 0);
        for (uint_t entries = stream.Read<le_uint32_t>() / 8; entries != 0; --entries)
        {
          const auto position = stream.Read<le_uint32_t>();
          const auto offset = stream.Read<le_uint32_t>();
          SampleProperties::LookupEntry entry(offset, position);
          if (dst.Lookup.back().Before(entry))
          {
            dst.Lookup.emplace_back(std::move(entry));
          }
          else
          {
            Dbg("Ignore invalid lookup {}@{}", position, offset);
          }
        }
        if (!dst.Setup)
        {
          Dbg("Unknown vorbis metadata for sample #{} ('{}') crc={}", CurSample, dst.Name, crc);
        }
        // Require(dst.Setup);
      }

      void SetData(uint_t samplesCount, Binary::Container::Ptr blob) override
      {
        auto& dst = Samples[CurSample];
        dst.SamplesCount = samplesCount;
        dst.Data = std::move(blob);
        SampleProperties::LookupEntry limiter(dst.Data->Size(), samplesCount);
        while (!dst.Lookup.empty())
        {
          const auto& last = dst.Lookup.back();
          if (last.Before(limiter))
          {
            break;
          }
          Dbg("Drop invalid lookup {}@{}", last.Position, last.Offset);
          dst.Lookup.pop_back();
        }
        dst.Lookup.emplace_back(std::move(limiter));
      }

      NamedDataMap CaptureResult() override
      {
        NamedDataMap result;
        for (auto& smp : Samples)
        {
          if (smp.Data && smp.Setup)
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
  }  // namespace Vorbis

  FormatBuilder::Ptr CreateOggVorbisBuilder()
  {
    return MakePtr<Vorbis::Builder>();
  }
}  // namespace Formats::Archived::FSB
