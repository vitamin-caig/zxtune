/**
* 
* @file
*
* @brief  MP3 parser implementation
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "mp3.h"
#include "formats/chiptune/container.h"
//common includes
#include <make_ptr.h>
//library includes
#include <binary/input_stream.h>
#include <binary/format_factories.h>
#include <strings/encoding.h>
#include <strings/trim.h>
//std includes
#include <array>
//text includes
#include <formats/text/chiptune.h>

namespace Formats
{
namespace Chiptune
{
  namespace Mp3
  {
    namespace Id3
    {
      StringView CutValid(StringView str)
      {
        const auto end = std::find_if(str.begin(), str.end(), [](Char c) {return c < ' ' && c != '\t' && c != '\r' && c != '\n';});
        return {str.begin(), end};
      }
    
      String MakeString(StringView str)
      {
        //do not trim before- it may break some encodings
        auto decoded = Strings::ToAutoUtf8(str);
        auto trimmed = Strings::TrimSpaces(CutValid(decoded));
        return decoded.size() == trimmed.size() ? decoded : trimmed.to_string();
      }

      //support V2.2+ only due to different tag size
      class V2Format
      {
      public:
        V2Format(const void* data, std::size_t size)
          : Stream(data, size)
        {
        }
        
        void Parse(MetaBuilder& target)
        {
          static const size_t HEADER_SIZE = 10;
          while (Stream.GetRestSize() >= HEADER_SIZE)
          {
            const auto header = Stream.ReadRawData(HEADER_SIZE);
            const auto id = ReadBE32(header);
            const std::size_t chunkize = ReadBE32(header + 4);
            const bool compressed = header[9] & 0x80;
            const bool encrypted = header[9] & 0x40;
            const auto body = Stream.ReadRawData(chunkize);
            if (!compressed && !encrypted && chunkize > 0)
            {
              ParseTag(id, body, chunkize, target);
            }
          }
        }
      private:
        static uint32_t ReadBE32(const uint8_t* data)
        {
          return (uint_t(data[0]) << 24) | (uint_t(data[1]) << 16) | (uint_t(data[2]) << 8) | data[3];
        }
        
        static void ParseTag(uint32_t id, const void* data, std::size_t size, MetaBuilder& target)
        {
          // http://id3.org/id3v2.3.0#Text_information_frames
          StringView encodedString(static_cast<const char*>(data) + 1, size - 1);
          switch (id)
          {
          case 0x54495432://'TIT2'
            target.SetTitle(MakeString(encodedString));
            break;
          case 0x54504531://'TPE1'
          case 0x544f5045://'TOPE'
            target.SetAuthor(MakeString(encodedString));
            break;
          case 0x434f4d4d://'COMM'
            //http://id3.org/id3v2.3.0#Comments
            encodedString = encodedString.substr(3);
          case 0x54585858://'TXXX'
            {
              // http://id3.org/id3v2.3.0#User_defined_text_information_frame
              const auto zeroPos = encodedString.find(0);
              Strings::Array strings;
              if (zeroPos != StringView::npos)
              {
                const auto descr = MakeString(encodedString.substr(0, zeroPos));
                if (!descr.empty())
                {
                  strings.push_back(descr);
                }
                const auto value = MakeString(encodedString.substr(zeroPos + 1));
                if (!value.empty())
                {
                  strings.push_back(value);
                }
              }
              else
              {
                const auto val = MakeString(encodedString);
                if (!val.empty())
                {
                  strings.push_back(val);
                }
              }
              if (!strings.empty())
              {
                target.SetStrings(strings);
              }
            }
            break;
          case 0x54535345://'TSSE'
          case 0x54454e43://'TENC'
            target.SetProgram(MakeString(encodedString));
            break;
          default:
            break;
          }
        }
      private:
        Binary::DataInputStream Stream;
      };
      
      using TagString = std::array<char, 30>;

      struct Tag
      {
        char Signature[3];
        TagString Title;
        TagString Artist;
        TagString Album;
        char Year[4];
        TagString Comment;
        uint8_t Genre;
      };
      
      static_assert(sizeof(Tag) == 128, "Invalid layout");
      
      using EnhancedTagString = std::array<char, 60>;
      
      struct EnhancedTag
      {
        char Signature[3];
        char PlusSign;
        EnhancedTagString Title;
        EnhancedTagString Artist;
        EnhancedTagString Album;
        uint8_t Speed;
        char Genre[30];
        uint8_t Start[6];
        uint8_t End[6];
      };
      
      static_assert(sizeof(EnhancedTag) == 227, "Invalid layout");

      String MakeString(const TagString& str)
      {
        return MakeString(StringView(str));
      }
      
      String MakeString(const EnhancedTagString& part1, const TagString& part2)
      {
        std::array<char, 90> buf;
        std::copy(part2.begin(), part2.end(), std::copy(part1.begin(), part1.end(), buf.begin()));
        return MakeString(StringView(buf));
      }
      
      //https://en.wikipedia.org/wiki/ID3#ID3v1_and_ID3v1.1
      bool ParseV1(Binary::InputStream& stream, MetaBuilder& target)
      {
        try
        {
          const Tag* tag = safe_ptr_cast<const Tag*>(stream.ReadRawData(sizeof(Tag)));
          const EnhancedTag* enhancedTag = nullptr;
          if (tag->Title[0] == '+')
          {
            stream.Skip(sizeof(EnhancedTag));
            enhancedTag = safe_ptr_cast<const EnhancedTag*>(tag);
            tag = safe_ptr_cast<const Tag*>(enhancedTag + 1);
          }
          if (enhancedTag)
          {
            target.SetTitle(MakeString(enhancedTag->Title, tag->Title));
            target.SetAuthor(MakeString(enhancedTag->Artist, tag->Artist));
          }
          else
          {
            target.SetTitle(MakeString(tag->Title));
            target.SetAuthor(MakeString(tag->Artist));
          }
          //TODO: add MetaBuilder::SetComment field
          {
            const auto comment = StringView(tag->Comment);
            const auto hasTrackNum = comment[28] == 0 || comment[28] == 0xff;//standard violation
            target.SetStrings({MakeString(hasTrackNum ? comment.substr(0, 28) : comment)});
          }
          return true;
        }
        catch (const std::exception&)
        {
        }
        return false;
      }
      
      struct Header
      {
        char Signature[3];
        uint8_t Major;
        uint8_t Revision;
        uint8_t Flags;
        uint8_t Size[4];
      };
      
      static_assert(sizeof(Header) == 10, "Invalid layout");
      
      bool ParseV2(Binary::InputStream& stream, MetaBuilder& target)
      {
        const auto& header = stream.ReadField<Header>();
        const uint_t tagSize = ((header.Size[0] & 0x7f) << 21) | ((header.Size[1] & 0x7f) << 14) | ((header.Size[2] & 0x7f) << 7) | (header.Size[3] & 0x7f);
        const auto content = stream.ReadRawData(tagSize);
        const bool hasExtendedHeader = header.Flags & 0x40;
        try
        {
          if (header.Major >= 3 && !hasExtendedHeader)
          {
            V2Format(content, tagSize).Parse(target);
          }
        }
        catch (const std::exception&)
        {
        }
        return true;
      }
      
      bool Parse(Binary::InputStream& stream, MetaBuilder& target)
      {
        static const size_t SIGNATURE_SIZE = 3;
        static const uint8_t V1_SIGNATURE[] = {'T', 'A', 'G'};
        static const uint8_t V2_SIGNATURE[] = {'I', 'D', '3'};
        if (stream.GetRestSize() < SIGNATURE_SIZE)
        {
          return false;
        }
        const auto sign = stream.PeekRawData(SIGNATURE_SIZE);
        if (0 == std::memcmp(sign, V1_SIGNATURE, sizeof(V1_SIGNATURE)))
        {
          return ParseV1(stream, target);
        }
        else if (0 == std::memcmp(sign, V2_SIGNATURE, sizeof(V2_SIGNATURE)))
        {
          return ParseV2(stream, target);
        }
        else
        {
          return false;
        }
      }
    } //namespace Id3
    
    class StubBuilder : public Builder
    {
    public:
      MetaBuilder& GetMetaBuilder() override
      {
        return GetStubMetaBuilder();
      }

      void AddFrame(const Frame& /*frame*/)
      {
      }
    };
    
    //as in minimp3
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
      
      bool IsValid() const
      {
        return CheckSync()
            && GetLayer() != Layer::RESERVED
            && GetBitrate() != Bitrate::INVALID
            && GetFrequency() != 3
        ;
      }
      
      bool GetIsMono() const
      {
        return (Data[3] & 0xc0) == 0xc0;
      }
      
      uint_t GetBitrateKbps() const
      {
        static const uint_t HALFRATE[2][3][15] =
        {
          //Mpeg2/2.5
          {
            //Layer III
            { 0,4,8,12,16,20,24,28,32,40,48,56,64,72,80 },
            //Layer II
            { 0,4,8,12,16,20,24,28,32,40,48,56,64,72,80 },
            //Layer I
            { 0,16,24,28,32,40,48,56,64,72,80,88,96,112,128 }
          },
          //Mpeg1
          {
            //Layer III
            { 0,16,20,24,28,32,40,48,56,64,80,96,112,128,160 },
            //Layer II
            { 0,16,24,28,32,40,48,56,64,80,96,112,128,160,192 },
            //Layer I
            { 0,16,32,48,64,80,96,112,128,144,160,176,192,208,224 }
          },
        };
        return 2 * HALFRATE[GetVersion() == Version::MPEG_1][GetLayer() - 1][GetBitrate()];
      }

      uint_t GetSamplerate() const
      {
        static const uint_t FREQUENCY[3] =
        { 
          44100, 48000, 32000
        };
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
    };
    
    static_assert(sizeof(FrameHeader) == 4, "Invalid layout");
    
    class Format
    {
    public:
      explicit Format(const Binary::Container& data)
        : Stream(data)
      {
      }
      
      Container::Ptr Parse(Builder& target)
      {
        Id3::Parse(Stream, target.GetMetaBuilder());
        for (;;)
        {
          const auto offset = Stream.GetPosition();
          if (const auto inFrame = FindFrame())
          {
            Frame out;
            out.Location.Offset = offset;
            out.Location.Size = Stream.GetPosition() - offset;
            out.Properties.Samplerate = inFrame->GetSamplerate();
            out.Properties.SamplesCount = inFrame->GetSamplesCount();
            out.Properties.Mono = inFrame->GetIsMono();
            target.AddFrame(out);
          }
          else if (Id3::Parse(Stream, target.GetMetaBuilder()))
          {
            continue;
          }
          else if (!SkipToNextFrame())
          {
            break;
          }
        }
        if (const auto subData = Stream.GetReadData())
        {
          return CreateCalculatingCrcContainer(subData, 0, subData->Size());
        }
        else
        {
          return Container::Ptr();
        }
      }
    private:
      const FrameHeader* FindFrame()
      {
        const auto frame = safe_ptr_cast<const FrameHeader*>(Stream.PeekRawData(sizeof(FrameHeader)));
        if (frame && frame->IsValid())
        {
          const auto size = frame->GetFrameDataSize();
          const auto restSize = Stream.GetRestSize();
          if (size != 0)
          {
            if (restSize >= size)
            {
              Stream.Skip(size);
              return frame;
            }
          }
          else
          {
            if (!SkipToNextFrame())
            {
              Stream.Skip(restSize);
            }
            return frame;
          }
        }
        return nullptr;
      }
      
      bool SkipToNextFrame()
      {
        const auto restSize = Stream.GetRestSize();
        if (restSize < sizeof(FrameHeader))
        {
          return false;
        }
        const auto data = Stream.PeekRawData(restSize);
        const auto end = data + restSize - sizeof(FrameHeader);
        for (std::size_t offset = 1; ; )
        {
          const auto match = std::find(data + offset, end, 0xff);
          if (match == end)
          {
            break;
          }
          else if (safe_ptr_cast<const FrameHeader*>(match)->IsValid())
          {
            Stream.Skip(match - data);
            return true;
          }
          else
          {
            offset = match - data + 1;
          }
        }
        return false;
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
        return Formats::Chiptune::Container::Ptr();
      }
    }
    
    Builder& GetStubBuilder()
    {
      static StubBuilder stub;
      return stub;
    }
    
    const std::string FORMAT(
      //ID3 tag    frame header
      "'I         |ff"
      "'D         |%111xxxxx"
      "'3         |%00000000-%11101011" //%(0000-1110)(00-10)xx really
      /* useless due to frame header signature end
      "02-04"
      "?"
      "%xxx00000"
      "%0xxxxxxx"
      "%0xxxxxxx"
      "%0xxxxxxx"
      "%0xxxxxxx"
      */
    );
    
    class Decoder : public Formats::Chiptune::Decoder
    {
    public:
      Decoder()
        : Format(Binary::CreateMatchOnlyFormat(FORMAT))
      {
      }

      String GetDescription() const override
      {
        return Text::MP3_DECODER_DESCRIPTION;
      }

      Binary::Format::Ptr GetFormat() const override
      {
        return Format;
      }

      bool Check(const Binary::Container& rawData) const override
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
  } //namespace Mp3

  Decoder::Ptr CreateMP3Decoder()
  {
    return MakePtr<Mp3::Decoder>();
  }
}
}
