/**
 *
 * @file
 *
 * @brief  MP3 parser implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include "formats/chiptune/music/mp3.h"
#include "formats/chiptune/container.h"
#include "formats/chiptune/music/tags_id3.h"
// common includes
#include <make_ptr.h>
#include <string_view.h>
// library includes
#include <binary/format_factories.h>
#include <binary/input_stream.h>
#include <strings/sanitize.h>
// std includes
#include <array>

namespace Formats::Chiptune
{
  namespace Mp3
  {
    const Char DESCRIPTION[] = "MPEG Audio Layer";

    // http://wiki.hydrogenaud.io/index.php?title=APEv2_specification
    namespace ApeTag
    {
      void ParseKey(StringView key, StringView value, MetaBuilder& target)
      {
        if (key == "Artist"sv)
        {
          target.SetAuthor(Strings::Sanitize(value));
        }
        else if (key == "Title"sv)
        {
          target.SetTitle(Strings::Sanitize(value));
        }
        else if (key == "Comment"sv)
        {
          target.SetComment(Strings::SanitizeMultiline(value));
        }
      }

      void ParseV2(uint_t count, Binary::DataInputStream& stream, MetaBuilder& target)
      {
        try
        {
          for (uint_t idx = 0; idx < count; ++idx)
          {
            const std::size_t dataSize = stream.Read<le_uint32_t>();
            /*const auto flags = */ stream.Read<le_uint32_t>();
            const auto avail = stream.GetRestSize();
            Require(avail >= dataSize);
            const auto key = stream.ReadCString(avail - dataSize);
            const auto data = stream.ReadData(dataSize);
            ParseKey(key, StringView(data.As<char>(), data.Size()), target);
          }
        }
        catch (const std::exception&)
        {}
      }

      bool Parse(Binary::DataInputStream& stream, MetaBuilder& target)
      {
        static const std::size_t HEADER_SIZE = 32;
        static const uint8_t SIGNATURE[] = {'A', 'P', 'E', 'T', 'A', 'G', 'E', 'X'};
        const auto* const hdr = stream.PeekRawData(HEADER_SIZE);
        if (!hdr || 0 != std::memcmp(hdr, SIGNATURE, sizeof(SIGNATURE)))
        {
          return false;
        }
        const auto version = ReadLE<uint32_t>(hdr + 8);
        const auto restSize = ReadLE<uint32_t>(hdr + 12);
        const auto itemsCount = ReadLE<uint32_t>(hdr + 16);
        // const auto globalFlags = ReadLE<uint32_t>(hdr + 20);
        if (stream.PeekRawData(HEADER_SIZE + restSize))
        {
          stream.Skip(HEADER_SIZE);
          const auto subData = stream.ReadData(restSize);
          if (version == 2000)
          {
            Binary::DataInputStream subStream(subData);
            ParseV2(itemsCount, subStream, target);
          }
          return true;
        }
        else
        {
          return false;
        }
      }
    }  // namespace ApeTag

    class StubBuilder : public Builder
    {
    public:
      MetaBuilder& GetMetaBuilder() override
      {
        return GetStubMetaBuilder();
      }

      void AddFrame(const Frame& /*frame*/) override {}
    };

    // as in minimp3
    struct FrameHeader
    {
      std::array<uint8_t, 4> Data;

      struct Version
      {
        enum
        {
          MPEG2_5 = 0,
          RESERVED = 1,
          MPEG_2 = 2,
          MPEG_1 = 3
        };
      };

      struct Layer
      {
        enum
        {
          RESERVED = 0,
          III = 1,
          II = 2,
          I = 3
        };
      };

      struct Bitrate
      {
        enum
        {
          FREE = 0x0,
          INVALID = 0xf
        };
      };

      struct Channels
      {
        enum
        {
          STEREO = 0,
          JOINT_STEREO = 1,
          DUAL = 2,
          MONO = 3
        };
      };

      bool IsValid() const
      {
        return CheckSync() && GetLayer() != Layer::RESERVED && GetBitrate() != Bitrate::INVALID && GetFrequency() != 3
               && GetChannels() != Channels::DUAL;
      }

      bool GetIsMono() const
      {
        return GetChannels() == Channels::MONO;
      }

      uint_t GetBitrateKbps() const
      {
        static const uint_t HALFRATE[2][3][15] = {
            // Mpeg2/2.5
            {// Layer III
             {0, 4, 8, 12, 16, 20, 24, 28, 32, 40, 48, 56, 64, 72, 80},
             // Layer II
             {0, 4, 8, 12, 16, 20, 24, 28, 32, 40, 48, 56, 64, 72, 80},
             // Layer I
             {0, 16, 24, 28, 32, 40, 48, 56, 64, 72, 80, 88, 96, 112, 128}},
            // Mpeg1
            {// Layer III
             {0, 16, 20, 24, 28, 32, 40, 48, 56, 64, 80, 96, 112, 128, 160},
             // Layer II
             {0, 16, 24, 28, 32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192},
             // Layer I
             {0, 16, 32, 48, 64, 80, 96, 112, 128, 144, 160, 176, 192, 208, 224}},
        };
        return 2 * HALFRATE[GetVersion() == Version::MPEG_1][GetLayer() - 1][GetBitrate()];
      }

      uint_t GetSamplerate() const
      {
        static const uint_t FREQUENCY[3] = {44100, 48000, 32000};
        const auto divider = GetVersion() == Version::MPEG2_5 ? 4 : (GetVersion() == Version::MPEG_2 ? 2 : 1);
        return FREQUENCY[GetFrequency()] / divider;
      }

      uint_t GetSamplesCount() const
      {
        const auto layer = GetLayer();
        if (layer == Layer::I)
        {
          return 384;
        }
        else if (layer == Layer::II || GetVersion() == Version::MPEG_1)
        {
          return 1152;
        }
        else
        {
          return 576;
        }
      }

      bool IsFreeFormat() const
      {
        return 0 == GetBitrate();
      }

      bool Matches(const FrameHeader& rh) const
      {
        return ((Data[1] ^ rh.Data[1]) & 0xFE) == 0 && ((Data[2] ^ rh.Data[2]) & 0x0C) == 0
               && (IsFreeFormat() == rh.IsFreeFormat());
      }

      std::size_t GetFrameDataSize() const
      {
        const auto res = GetSamplesCount() * (GetBitrateKbps() * 1000 / 8) / GetSamplerate();
        const auto padding = (Data[2] & 0x2) != 0;
        if (GetLayer() == Layer::I)
        {
          return (res & ~3) + 4 * padding;
        }
        else
        {
          return res + padding;
        }
      }

    private:
      bool CheckSync() const
      {
        // Mpeg2.5 only LayerIII supported
        return Data[0] == 0xff && ((Data[1] & 0xf0) == 0xf0 || (Data[1] & 0xfe) == 0xe2);
      }

      uint_t GetVersion() const
      {
        return (Data[1] & 0x18) >> 3;
      }

      uint_t GetLayer() const
      {
        return (Data[1] & 0x06) >> 1;
      }

      uint_t GetBitrate() const
      {
        return Data[2] >> 4;
      }

      uint_t GetFrequency() const
      {
        return (Data[2] & 0x0c) >> 2;
      }

      uint_t GetChannels() const
      {
        return (Data[3] & 0xc0) >> 6;
      }
    };

    static_assert(sizeof(FrameHeader) == 4, "Invalid layout");
    static const std::size_t MIN_FREEFORMAT_FRAME_SIZE = 16;
    static const std::size_t MAX_FREEFORMAT_FRAME_SIZE = 2304;
    static const uint_t MAX_SYNC_GAP_NOTLOST = 3;
    static const uint_t MAX_SYNC_LOSTS_COUNT = 3;

    class Format
    {
    public:
      explicit Format(const Binary::Container& data)
        : Stream(data)
      {}

      Container::Ptr Parse(Builder& target)
      {
        auto* metaTarget = &target.GetMetaBuilder();
        if (Id3::Parse(Stream, *metaTarget))
        {
          metaTarget = &GetStubMetaBuilder();
        }

        uint_t syncLostsCount = 0;
        for (auto freeFormatFrameSize = Synchronize(); Stream.GetRestSize() != 0;)
        {
          const auto offset = Stream.GetPosition();
          if (const auto* const inFrame = ReadFrame(freeFormatFrameSize))
          {
            Frame out;
            out.Location.Offset = offset;
            out.Location.Size = Stream.GetPosition() - offset;
            out.Properties.Samplerate = inFrame->GetSamplerate();
            out.Properties.SamplesCount = inFrame->GetSamplesCount();
            out.Properties.Mono = inFrame->GetIsMono();
            target.AddFrame(out);
          }
          else if (Id3::Parse(Stream, *metaTarget) || ApeTag::Parse(Stream, *metaTarget))
          {
            metaTarget = &GetStubMetaBuilder();
            continue;
          }
          else
          {
            freeFormatFrameSize = Synchronize();
            const auto skip = Stream.GetPosition() - offset;
            if (!skip)
            {
              // nothing found
              break;
            }
            if (skip > MAX_SYNC_GAP_NOTLOST && ++syncLostsCount > MAX_SYNC_LOSTS_COUNT)
            {
              return {};
            }
          }
        }
        if (const auto subData = Stream.GetReadContainer())
        {
          return CreateCalculatingCrcContainer(subData, 0, subData->Size());
        }
        else
        {
          return {};
        }
      }

    private:
      //@return free format frame size
      std::size_t Synchronize()
      {
        const std::size_t FREEFORMAT_SYNC_FRAMES_COUNT = 10;
        while (const auto* const firstFrame = SkipToNextFrame(0))
        {
          if (!firstFrame->IsFreeFormat())
          {
            break;
          }
          const auto startOffset = Stream.GetPosition();
          std::size_t frameSize = 0;
          while (const auto* const nextFrame = SkipToNextFrame(sizeof(*firstFrame)))
          {
            if (nextFrame->Matches(*firstFrame))
            {
              frameSize = Stream.GetPosition() - startOffset;
              break;
            }
          }
          for (std::size_t validFrames = 2;
               frameSize > MIN_FREEFORMAT_FRAME_SIZE && frameSize < MAX_FREEFORMAT_FRAME_SIZE; ++validFrames)
          {
            const auto* const nextFrame = SkipToNextFrame(frameSize);
            if (!nextFrame || validFrames >= FREEFORMAT_SYNC_FRAMES_COUNT)
            {
              Stream.Seek(startOffset);
              return frameSize;
            }
            else if (!nextFrame->Matches(*firstFrame) || 0 != (Stream.GetPosition() - startOffset) % frameSize)
            {
              break;
            }
          }
          Stream.Seek(startOffset + 1);
        }
        return 0;
      }

      const FrameHeader* ReadFrame(std::size_t freeFormatSize)
      {
        const auto* const frame = safe_ptr_cast<const FrameHeader*>(Stream.PeekRawData(sizeof(FrameHeader)));
        if (frame && frame->IsValid())
        {
          if (frame->IsFreeFormat())
          {
            const auto restSize = Stream.GetRestSize();
            if (!freeFormatSize || restSize < freeFormatSize)
            {
              return nullptr;
            }
            else if (const auto* const nextFrameStart = Stream.PeekRawData(freeFormatSize + freeFormatSize))
            {
              if (!safe_ptr_cast<const FrameHeader*>(nextFrameStart)->Matches(*frame))
              {
                return nullptr;
              }
            }
            Stream.Skip(freeFormatSize);
            return frame;
          }
          else
          {
            const auto size = frame->GetFrameDataSize();
            const auto restSize = Stream.GetRestSize();
            if (restSize >= size)
            {
              Stream.Skip(size);
              return frame;
            }
          }
        }
        return nullptr;
      }

      const FrameHeader* SkipToNextFrame(std::size_t startOffset)
      {
        const auto restSize = Stream.GetRestSize();
        if (restSize < startOffset + sizeof(FrameHeader))
        {
          return nullptr;
        }
        const auto* const data = Stream.PeekRawData(restSize);
        const auto* const end = data + restSize - sizeof(FrameHeader);
        for (std::size_t offset = startOffset;;)
        {
          const auto* const match = std::find(data + offset, end, 0xff);
          if (match == end)
          {
            break;
          }
          else if (safe_ptr_cast<const FrameHeader*>(match)->IsValid())
          {
            Stream.Skip(match - data);
            return safe_ptr_cast<const FrameHeader*>(match);
          }
          else
          {
            offset = match - data + 1;
          }
        }
        return nullptr;
      }

    private:
      Binary::InputStream Stream;
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

    Builder& GetStubBuilder()
    {
      static StubBuilder stub;
      return stub;
    }

    const auto FORMAT =
        // ID3 tag    frame header
        "'I         |ff"
        "'D         |%111xxxxx"
        "'3         |%00000000-%11101011"  //%(0000-1110)(00-10)xx really
                                           /* useless due to frame header signature end
                                           "02-04"
                                           "?"
                                           "%xxx00000"
                                           "%0xxxxxxx"
                                           "%0xxxxxxx"
                                           "%0xxxxxxx"
                                           "%0xxxxxxx"
                                           */
        ""sv;

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
          return {};
        }
      }

    private:
      const Binary::Format::Ptr Format;
    };
  }  // namespace Mp3

  Decoder::Ptr CreateMP3Decoder()
  {
    return MakePtr<Mp3::Decoder>();
  }
}  // namespace Formats::Chiptune
