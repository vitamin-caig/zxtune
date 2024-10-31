/**
 *
 * @file
 *
 * @brief  Ogg Vorbis parser implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "formats/chiptune/music/oggvorbis.h"

#include "formats/chiptune/builder_meta.h"
#include "formats/chiptune/container.h"
#include "formats/chiptune/music/tags_vorbis.h"

#include "binary/data_builder.h"
#include "binary/format.h"
#include "binary/format_factories.h"
#include "binary/input_stream.h"

#include "byteorder.h"
#include "contract.h"
#include "make_ptr.h"
#include "string_view.h"

#include <algorithm>
#include <array>
#include <cstring>
#include <exception>
#include <iterator>
#include <memory>
#include <numeric>
#include <unordered_map>
#include <utility>
#include <vector>

namespace Formats::Chiptune
{
  namespace Ogg
  {
    const uint8_t SIGNATURE[] = {'O', 'g', 'g', 'S', 0};
    const uint64_t UNFINISHED_PAGE_POSITION = ~0uLL;
    const uint_t MAX_SEGMENT_SIZE = 255;
    const uint_t MAX_PAGE_SIZE = 32768;
    const uint_t CONTINUED_PACKET = 1;
    const uint_t FIRST_PAGE = 2;
    const uint_t LAST_PAGE = 4;

    const uint8_t* SeekSignature(Binary::DataInputStream& stream, const uint8_t* needle, std::size_t size)
    {
      const auto avail = stream.GetRestSize();
      if (avail < size)
      {
        return nullptr;
      }
      const auto* start = stream.PeekRawData(avail);
      const auto* end = start + avail;
      const auto* pos = std::search(start, end, needle, needle + size);
      if (pos != end)
      {
        stream.Skip(pos - start);
        return pos;
      }
      return nullptr;
    }

    // https://xiph.org/ogg/doc/framing.html
    class Format
    {
    public:
      explicit Format(const Binary::Container& data)
        : Stream(data)
      {}

      class Callback
      {
      public:
        virtual ~Callback() = default;

        virtual void OnPage(uint32_t streamId, std::size_t offset, uint64_t positionInFrames, Binary::View data) = 0;
      };

      Binary::Container::Ptr Parse(Callback& target)
      {
        static const std::size_t MIN_PAGE_SIZE = 27;
        class StreamData
        {
        public:
          explicit StreamData(uint32_t id)
            : Id(id)
          {}

          uint32_t GetId() const
          {
            return Id;
          }

          void StartPage(uint32_t number)
          {
            Require(NextPageNumber <= number);
            NextPageNumber = number + 1;
          }

          void AddPart(std::size_t offset, uint64_t position, Binary::View part)
          {
            if (Parts.empty())
            {
              Offset = offset;
              Position = position;
            }
            Parts.push_back(part);
          }

          void Flush(Callback& target)
          {
            if (Parts.size() == 1)
            {
              target.OnPage(Id, Offset, Position, Parts.front());
            }
            else if (!Parts.empty())
            {
              target.OnPage(Id, Offset, Position, MergeParts());
            }
            Parts.clear();
          }

        private:
          Binary::View MergeParts()
          {
            const auto totalSize =
                std::accumulate(Parts.begin(), Parts.end(), std::size_t(0),
                                [](std::size_t sum, Binary::View part) { return sum + part.Size(); });
            Buffer = Binary::DataBuilder(totalSize);
            for (const auto& p : Parts)
            {
              Buffer.Add(p);
            }
            return Buffer.GetView();
          }

        private:
          const uint32_t Id;
          uint32_t NextPageNumber = 0;
          std::size_t Offset = 0;
          uint64_t Position = 0;
          std::vector<Binary::View> Parts;
          Binary::DataBuilder Buffer;
        };
        std::vector<StreamData> streams;
        StreamData* currentStream = nullptr;
        while (Stream.GetRestSize() >= MIN_PAGE_SIZE && SyncToPage())
        {
          const auto offset = Stream.GetPosition();
          Stream.Skip(sizeof(SIGNATURE));
          const auto flags = Stream.ReadByte();
          const auto newPacket = (flags & CONTINUED_PACKET) == 0;
          const uint64_t position = Stream.Read<le_uint64_t>();
          const uint_t streamId = Stream.Read<le_uint32_t>();
          if (currentStream && currentStream->GetId() != streamId)
          {
            currentStream = nullptr;
            for (auto& s : streams)
            {
              if (s.GetId() == streamId)
              {
                currentStream = &s;
                break;
              }
            }
          }
          if (!currentStream)
          {
            streams.emplace_back(streamId);
            currentStream = &streams.back();
          }
          currentStream->StartPage(Stream.Read<le_uint32_t>());
          /*const auto crc = */ Stream.Read<le_uint32_t>();
          const auto segmentsCount = Stream.ReadByte();
          const auto* const segmentsSizes = Stream.PeekRawData(segmentsCount);
          if (!segmentsSizes)
          {
            Stream.Seek(offset);
            break;
          }
          Stream.Skip(segmentsCount);
          if (const auto payloadSize = std::accumulate(segmentsSizes, segmentsSizes + segmentsCount, std::size_t(0)))
          {
            if (newPacket)
            {
              currentStream->Flush(target);
            }
            currentStream->AddPart(offset, position, Stream.ReadData(std::min(payloadSize, Stream.GetRestSize())));
          }
        }
        if (currentStream)
        {
          currentStream->Flush(target);
        }
        return Stream.GetReadContainer();
      }

    private:
      bool SyncToPage()
      {
        if (0 == std::memcmp(Stream.PeekRawData(sizeof(SIGNATURE)), SIGNATURE, sizeof(SIGNATURE)))
        {
          return true;
        }
        return SeekSignature(Stream, SIGNATURE, sizeof(SIGNATURE));
      }

    private:
      Binary::InputStream Stream;
    };
  }  // namespace Ogg

  // https://xiph.org/vorbis/doc/Vorbis_I_spec.html
  namespace Vorbis
  {
    const uint32_t VERSION = 0;
    const std::array<uint8_t, 6> SIGNATURE = {{'v', 'o', 'r', 'b', 'i', 's'}};

    enum PacketType
    {
      Identification = 1,
      Comment = 3,
      Setup = 5,
      Audio = 0  // arbitrary
    };

    class Format : public Ogg::Format::Callback
    {
    public:
      explicit Format(OggVorbis::Builder& target)
        : Target(target)
      {}

      void OnPage(uint32_t streamId, std::size_t offset, uint64_t positionInFrames, Binary::View data) override
      {
        if (streamId != LastStreamId)
        {
          LastStreamId = streamId;
          Target.SetStreamId(streamId);
          NextPacketType = &StreamsNextPacketTypes.try_emplace(streamId, Identification).first->second;
        }
        Binary::DataInputStream payload(data);
        while (payload.GetRestSize())
        {
          ReadPacket(offset, positionInFrames, payload);
        }
      }

    private:
      void ReadPacket(std::size_t pageOffset, uint64_t positionInFrames, Binary::DataInputStream& payload)
      {
        if (*NextPacketType != Audio)
        {
          if (const auto* type = FindHeaderPacket(*NextPacketType, payload))
          {
            Require(*NextPacketType == *type);
            ReadHeaderPacket(*type, payload);
          }
          else
          {
            Target.AddUnknownPacket(payload.ReadRestData());
          }
        }
        else
        {
          Target.AddFrame(pageOffset, positionInFrames, payload.ReadRestData());
        }
      }

      static const uint8_t* FindHeaderPacket(uint8_t type, Binary::DataInputStream& payload)
      {
        const uint8_t needle[] = {type, 'v', 'o', 'r', 'b', 'i', 's'};
        if (const auto* sign = Ogg::SeekSignature(payload, needle, std::size(needle)))
        {
          payload.Skip(std::size(needle));
          return sign;
        }
        return nullptr;
      }

      void ReadHeaderPacket(uint_t type, Binary::DataInputStream& payload)
      {
        switch (type)
        {
        case Identification:
          ReadIdentification(payload);
          *NextPacketType = Comment;
          break;
        case Comment:
          ParseComment(payload, Target.GetMetaBuilder());
          *NextPacketType = Setup;
          break;
        case Setup:
          payload.Seek(payload.GetPosition() - SIGNATURE.size() - 1);
          Target.SetSetup(payload.ReadRestData());
          *NextPacketType = Audio;
          break;
        default:
          Require(false);
        }
      }

      void ReadIdentification(Binary::DataInputStream& payload)
      {
        const uint_t version = payload.Read<le_uint32_t>();
        const auto channels = payload.ReadByte();
        const uint_t frequency = payload.Read<le_uint32_t>();
        payload.Skip(4 * 3);
        const auto blocksize = payload.ReadByte();
        const auto framing = payload.ReadByte();
        const auto blockLo = blocksize & 0x0f;
        const auto blockHi = blocksize >> 4;
        Require(version == VERSION);
        Require(channels > 0);
        Require(frequency > 0);
        Require(blockLo >= 6 && blockLo <= 13);
        Require(blockHi >= 6 && blockHi <= 13);
        Require(blockLo <= blockHi);
        Require(framing & 1);
        Target.SetProperties(channels, frequency, 1 << blockLo, 1 << blockHi);
      }

    private:
      OggVorbis::Builder& Target;
      uint32_t LastStreamId = ~0u;
      std::unordered_map<uint32_t, PacketType> StreamsNextPacketTypes;
      PacketType* NextPacketType = nullptr;
    };
  }  // namespace Vorbis

  namespace OggVorbis
  {
    const auto DESCRIPTION = "OGG Vorbis"sv;

    Formats::Chiptune::Container::Ptr Parse(const Binary::Container& data, Builder& target)
    {
      try
      {
        Ogg::Format ogg(data);
        Vorbis::Format vorbis(target);
        if (const auto subData = ogg.Parse(vorbis))
        {
          return CreateCalculatingCrcContainer(subData, 0, subData->Size());
        }
        else
        {
          return {};
        }
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

      void SetStreamId(uint32_t /*streamId*/) override {}
      void AddUnknownPacket(Binary::View /*data*/) override {}
      void SetProperties(uint_t /*channels*/, uint_t /*frequency*/, uint_t /*blockSizeLo*/,
                         uint_t /*blockSizeHi*/) override
      {}
      void SetSetup(Binary::View /*data*/) override {}
      void AddFrame(std::size_t /*offset*/, uint64_t /*positionInFrames*/, Binary::View /*data*/) override {}
    };

    Builder& GetStubBuilder()
    {
      static StubBuilder stub;
      return stub;
    }

    // Check only OGG container
    const auto FORMAT =
        "'O'g'g'S"  // signature
        "00"        // version
        "02"        // flags, first page of logical bitstream
        "00{8}"     // position
        "?{4}"      // serial
        "00000000"  // page
        ""sv;

    class Decoder : public Formats::Chiptune::Decoder
    {
    public:
      Decoder()
        : Format(Binary::CreateMatchOnlyFormat(FORMAT))
      {}

      StringView GetDescription() const override
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
  }  // namespace OggVorbis

  Decoder::Ptr CreateOGGDecoder()
  {
    return MakePtr<OggVorbis::Decoder>();
  }
}  // namespace Formats::Chiptune
