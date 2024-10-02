/**
 *
 * @file
 *
 * @brief  V2m parser implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include "formats/chiptune/digital/v2m.h"
#include "formats/chiptune/container.h"
// common includes
#include <byteorder.h>
#include <make_ptr.h>
// library includes
#include <binary/format_factories.h>
#include <binary/input_stream.h>
#include <math/numeric.h>

namespace Formats::Chiptune
{
  namespace V2m
  {
    const Char DESCRIPTION[] = "Farbrausch V2 Synthesizer System";

    class Format
    {
    public:
      explicit Format(const Binary::Container& data)
        : Stream(data)
      {}

      Container::Ptr Parse(Builder& target)
      {
        if (ParseHeader(target) && ParseChannels() && ParseData())
        {
          if (const auto subData = Stream.GetReadContainer())
          {
            return CreateCalculatingCrcContainer(subData, 0, subData->Size());
          }
        }
        return {};
      }

    private:
      bool ParseHeader(Builder& target)
      {
        Header hdr;
        if (hdr.Read(Stream))
        {
          target.SetTotalDuration(hdr.GetDuration());
          return true;
        }
        return false;
      }

      bool ParseChannels()
      {
        for (auto i = 0; i < 16; ++i)
        {
          if (const uint_t notes = Stream.Read<le_uint32_t>())
          {
            Stream.Skip(5 * notes);
            Stream.Skip(4 * Stream.Read<le_uint32_t>());
            Stream.Skip(5 * Stream.Read<le_uint32_t>());
            for (auto j = 0; j < 7; ++j)
            {
              Stream.Skip(4 * Stream.Read<le_uint32_t>());
            }
          }
        }
        return true;
      }

      bool ParseData()
      {
        const std::size_t globalsSize = Stream.Read<le_uint32_t>();
        if (globalsSize > 131072)
        {
          return false;
        }
        Stream.Skip(globalsSize);
        const std::size_t patchMapSize = Stream.Read<le_uint32_t>();
        if (patchMapSize > 1048576)
        {
          return false;
        }
        Stream.Skip(patchMapSize);
        if (const auto* const speech = Stream.PeekRawData(sizeof(uint32_t)))
        {
          const auto pos = Stream.GetPosition();
          const uint_t speechSize = Stream.Read<le_uint32_t>();
          if (Math::InRange<uint_t>(speechSize, 4, 8191))
          {
            const auto realSpeechSize = std::min<uint_t>(speechSize, Stream.GetRestSize());
            Binary::DataInputStream payload(Stream.ReadData(realSpeechSize));
            if (ParseSpeechData(payload))
            {
              return true;
            }
          }
          Stream.Seek(pos);
        }
        return true;
      }

      static bool ParseSpeechData(Binary::DataInputStream& stream)
      {
        const auto maxOffset = stream.GetRestSize() - 1;
        const uint_t count = stream.Read<le_uint32_t>();
        const auto minOffset = (count + 1) * sizeof(uint32_t);
        if (minOffset >= maxOffset)
        {
          return false;
        }
        for (uint_t idx = 0; idx < count; ++idx)
        {
          const uint_t offset = stream.Read<le_uint32_t>();
          if (!Math::InRange<uint_t>(offset, minOffset, maxOffset))
          {
            return false;
          }
        }
        return true;
      }

    private:
      Binary::InputStream Stream;
      struct Header
      {
        uint_t TimeDiv = 0;
        uint_t MaxTime = 0;
        uint_t GdNum = 0;
        const uint8_t* Delays = nullptr;

        bool Read(Binary::InputStream& stream)
        {
          // According to existing files' analyze
          static const uint_t MIN_TIMEDIV = 32;
          static const uint_t MAX_TIMEDIV = 0x1e0;
          static const uint_t MIN_MAXTIME = 256;
          static const uint_t MAX_MAXTIME = 0x00ffffff;
          static const uint_t MIN_GDNUM = 1;
          static const uint_t MAX_GDNUM = 6;

          TimeDiv = stream.Read<le_uint32_t>();
          MaxTime = stream.Read<le_uint32_t>();
          GdNum = stream.Read<le_uint32_t>();

          if (Math::InRange<uint_t>(TimeDiv, MIN_TIMEDIV, MAX_TIMEDIV)
              && Math::InRange<uint_t>(MaxTime, MIN_MAXTIME, MAX_MAXTIME)
              && Math::InRange<uint_t>(GdNum, MIN_GDNUM, MAX_GDNUM))
          {
            Delays = stream.ReadData(10 * GdNum).As<uint8_t>();
            return true;
          }
          return false;
        }

        Time::Milliseconds GetDuration() const
        {
          // See CalcPositions
          const uint64_t TPC = 1000;
          uint64_t totalTime = 0;
          for (uint_t gdIdx = 0, usecs = 500000, time = 0; gdIdx <= GdNum; ++gdIdx)
          {
            const auto delta = gdIdx < GdNum ? (uint_t(Delays[2 * GdNum + gdIdx]) << 16)
                                                   + (uint_t(Delays[GdNum + gdIdx]) << 8) + Delays[gdIdx]
                                             : MaxTime - time;
            time += delta;
            const auto rows = delta * 8 / TimeDiv;
            totalTime += TPC * rows * usecs / 8000000;
            if (gdIdx < GdNum)
            {
              usecs = safe_ptr_cast<const le_uint32_t*>(Delays + 3 * GdNum)[gdIdx];
            }
          }
          return Time::Milliseconds{static_cast<uint_t>(totalTime)};
        }
      };
    };

    Formats::Chiptune::Container::Ptr Parse(const Binary::Container& data, Builder& target)
    {
      try
      {
        return Format(data).Parse(target);
      }
      catch (const std::exception&)
      {
        return {};
      }
    }

    class StubBuilder : public Builder
    {
    public:
      MetaBuilder& GetMetaBuilder() override
      {
        return GetStubMetaBuilder();
      }

      void SetTotalDuration(Time::Milliseconds /*duration*/) override {}
    };

    Builder& GetStubBuilder()
    {
      static StubBuilder stub;
      return stub;
    }

    const auto FORMAT =
        "%xxx00000 00-01 0000"  // timediv
        "? 01-ff ? 00"          // maxtime
        "01-06 000000"          // gdnum
        ""sv;

    class Decoder : public Formats::Chiptune::Decoder
    {
    public:
      Decoder()
        : Format(Binary::CreateFormat(FORMAT))
      {}

      String GetDescription() const override
      {
        return DESCRIPTION;
      }

      Binary::Format::Ptr GetFormat() const override
      {
        return Format;
      }

      bool Check(Binary::View rawData) const override
      {
        return Format->Match(rawData);
      }

      Formats::Chiptune::Container::Ptr Decode(const Binary::Container& rawData) const override
      {
        if (Format->Match(rawData))
        {
          return Parse(rawData, GetStubBuilder());
        }
        else
        {
          return {};
        }
      }

    private:
      const Binary::Format::Ptr Format;
    };
  }  // namespace V2m

  Decoder::Ptr CreateV2MDecoder()
  {
    return MakePtr<V2m::Decoder>();
  }
}  // namespace Formats::Chiptune
