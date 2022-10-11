/**
 *
 * @file
 *
 * @brief  Ogg Vorbis parser implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include "formats/chiptune/music/oggvorbis.h"
#include "formats/chiptune/container.h"
#include "formats/chiptune/music/tags_vorbis.h"
// common includes
#include <byteorder.h>
#include <make_ptr.h>
// library includes
#include <binary/data_builder.h>
#include <binary/format_factories.h>
#include <binary/input_stream.h>
#include <math/bitops.h>
#include <strings/encoding.h>
#include <strings/trim.h>
// std includes
#include <cctype>
#include <numeric>
#include <unordered_map>

namespace Formats::Chiptune
{
  namespace Ogg
  {
    const uint8_t SIGNATURE[] = {'O', 'g', 'g', 'S', 0};
    const uint64_t UNFINISHED_PAGE_POSITION = ~0ull;
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
            Buffer.resize(totalSize);
            auto* target = Buffer.data();
            for (const auto& p : Parts)
            {
              std::memcpy(target, p.Start(), p.Size());
              target += p.Size();
            }
            return Buffer;
          }

        private:
          const uint32_t Id;
          uint32_t NextPageNumber = 0;
          std::size_t Offset = 0;
          uint64_t Position = 0;
          std::vector<Binary::View> Parts;
          Binary::Dump Buffer;
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
          const auto segmentsSizes = Stream.PeekRawData(segmentsCount);
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
        currentStream->Flush(target);
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

    class Builder
    {
    public:
      explicit Builder(std::size_t sizeHint)
        : Storage(sizeHint)
      {}

      void SetStreamId(uint32_t streamId)
      {
        StreamId = streamId;
      }

      void AddData(uint64_t position, const uint8_t* data, std::size_t size)
      {
        bool continued = false;
        while (size > MAX_PAGE_SIZE)
        {
          AddPage(position ? UNFINISHED_PAGE_POSITION : position, Binary::View(data, MAX_PAGE_SIZE), continued);
          data += MAX_PAGE_SIZE;
          size -= MAX_PAGE_SIZE;
          continued = true;
        }
        AddPage(position, Binary::View(data, size), continued);
      }

      Binary::Container::Ptr CaptureResult()
      {
        auto& flags = Storage.Get<uint8_t>(LastPageOffset + 5);
        flags |= LAST_PAGE;
        CalculateCrc();
        return Storage.CaptureResult();
      }

    private:
      void AddPage(uint64_t position, Binary::View data, bool continued)
      {
        if (PagesDone)
        {
          CalculateCrc();
        }
        const uint8_t segmentsCount = static_cast<uint8_t>(data.Size() / MAX_SEGMENT_SIZE) + 1;
        LastPageOffset = Storage.Size();
        LastPageSize = 27 + segmentsCount + data.Size();
        Storage.Allocate(LastPageSize);
        Storage.Resize(LastPageOffset);
        Storage.Add(SIGNATURE);
        // assume single stream, so first page is always first
        const uint8_t flag = PagesDone == 0 ? FIRST_PAGE : (continued ? CONTINUED_PACKET : 0);
        Storage.Add(flag);
        Storage.Add<le_uint64_t>(position);
        Storage.Add<le_uint32_t>(StreamId);
        Storage.Add<le_uint32_t>(PagesDone++);
        const le_uint32_t EMPTY_CRC = 0;
        Storage.Add(EMPTY_CRC);
        Storage.Add(segmentsCount);
        WriteSegments(data.Size(), static_cast<uint8_t*>(Storage.Allocate(segmentsCount)));
        Storage.Add(data);
      }

      void WriteSegments(std::size_t size, uint8_t* data)
      {
        for (;;)
        {
          const auto part = std::min<uint_t>(MAX_SEGMENT_SIZE, size);
          *data++ = static_cast<uint8_t>(part);
          size -= part;
          if (!size)
          {
            if (part == MAX_SEGMENT_SIZE)
            {
              *data++ = 0;
            }
            break;
          }
        }
      }

      void CalculateCrc()
      {
        auto* const page = static_cast<uint8_t*>(Storage.Get(LastPageOffset));
        auto* const rawCrc = safe_ptr_cast<le_uint32_t*>(page + 22);
        *rawCrc = 0;
        *rawCrc = Crc32::Calculate(page, LastPageSize);
      }

      class Crc32
      {
      public:
        Crc32()
        {
          const uint32_t CRC32_POLY = 0x04c11db7;
          for (uint_t i = 0; i < 256; ++i)
          {
            uint_t s = i << 24;
            for (uint_t j = 0; j < 8; ++j)
              s = (s << 1) ^ (s >= (1u << 31) ? CRC32_POLY : 0);
            Table[i] = s;
          }
        }

        static uint32_t Calculate(const uint8_t* data, std::size_t size)
        {
          static const Crc32 INSTANCE;
          uint32_t crc = 0;
          for (; size != 0; --size, ++data)
          {
            crc = (crc << 8) ^ INSTANCE.Table[*data ^ (crc >> 24)];
          }
          return crc;
        }

      private:
        uint32_t Table[256];
      };

    private:
      Binary::DataBuilder Storage;
      uint32_t StreamId = 0x2054585a;
      uint32_t PagesDone = 0;
      std::size_t LastPageOffset = 0;
      std::size_t LastPageSize = 0;
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
    const Char DESCRIPTION[] = "OGG Vorbis";

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
          return Container::Ptr();
        }
      }
      catch (const std::exception&)
      {
        return Formats::Chiptune::Container::Ptr();
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

    class SimpleDumpBuilder : public DumpBuilder
    {
    public:
      explicit SimpleDumpBuilder(std::size_t sizeHint)
        : Storage(sizeHint)
      {}

      MetaBuilder& GetMetaBuilder() override
      {
        return GetStubMetaBuilder();
      }

      void SetStreamId(uint32_t id) override
      {
        Storage.SetStreamId(id);
      }

      void AddUnknownPacket(Binary::View data) override
      {
        Storage.AddData(0, data.As<uint8_t>(), data.Size());
      }

      void SetProperties(uint_t channels, uint_t frequency, uint_t blockSizeLo, uint_t blockSizeHi) override
      {
        const auto blockSizes = (Math::Log2(blockSizeHi - 1) << 4) | (Math::Log2(blockSizeLo - 1));
        WriteIdentification(channels, frequency, blockSizes);
        WriteComment();
      }

      void SetSetup(Binary::View data) override
      {
        Storage.AddData(0, data.As<uint8_t>(), data.Size());
      }

      void AddFrame(std::size_t /*offset*/, uint64_t positionInFrames, Binary::View data) override
      {
        TotalFrames = positionInFrames;
        Storage.AddData(TotalFrames, data.As<uint8_t>(), data.Size());
      }

      Binary::Container::Ptr GetDump() override
      {
        return Storage.CaptureResult();
      }

    private:
      void WriteIdentification(uint8_t channels, uint32_t frequency, uint8_t blockSizes)
      {
        Binary::DataBuilder builder(30);
        builder.Add<uint8_t>(Vorbis::Identification);
        builder.Add(Vorbis::SIGNATURE);
        builder.Add<le_uint32_t>(Vorbis::VERSION);
        builder.Add(channels);
        builder.Add<le_uint32_t>(frequency);
        builder.Allocate(3 * sizeof(uint32_t));
        builder.Add(blockSizes);
        builder.Add<uint8_t>(1);
        assert(builder.Size() == 30);
        Storage.AddData(0, static_cast<const uint8_t*>(builder.Get(0)), builder.Size());
      }

      void WriteComment()
      {
        static const uint8_t DATA[] = {
            Vorbis::Comment, 'v', 'o', 'r', 'b', 'i', 's', 6, 0, 0, 0, 'z', 'x', 't', 'u', 'n', 'e', 0, 0, 0, 0, 1};
        Storage.AddData(0, DATA, sizeof(DATA));
      }

    private:
      Ogg::Builder Storage;
      uint64_t TotalFrames = 0;
    };

    DumpBuilder::Ptr CreateDumpBuilder(std::size_t sizeHint)
    {
      return MakePtr<SimpleDumpBuilder>(sizeHint);
    }

    // Check only OGG container
    const auto FORMAT =
        "'O'g'g'S"  // signature
        "00"        // version
        "02"        // flags, first page of logical bitstream
        "00{8}"     // position
        "?{4}"      // serial
        "00000000"  // page
        ""_sv;

    class Decoder : public Formats::Chiptune::Decoder
    {
    public:
      Decoder()
        : Format(Binary::CreateMatchOnlyFormat(FORMAT))
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
          return Formats::Chiptune::Container::Ptr();
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
