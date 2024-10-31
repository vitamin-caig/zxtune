/**
 *
 * @file
 *
 * @brief  ID3 tags parser implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "formats/chiptune/music/tags_id3.h"

#include <strings/sanitize.h>

#include <string_view.h>

#include <array>

namespace Formats::Chiptune::Id3
{
  // support V2.2+ only due to different tag size
  class V2Format
  {
  public:
    explicit V2Format(Binary::View data)
      : Stream(data)
    {}

    void Parse(MetaBuilder& target)
    {
      static const size_t HEADER_SIZE = 10;
      while (Stream.GetRestSize() >= HEADER_SIZE)
      {
        const uint_t id = Stream.Read<be_uint32_t>();
        const std::size_t chunkize = Stream.Read<be_uint32_t>();
        const uint_t flags = Stream.Read<be_uint16_t>();
        const bool compressed = flags & 0x80;
        const bool encrypted = flags & 0x40;
        const auto body = Stream.ReadData(chunkize);
        if (!compressed && !encrypted && chunkize > 0)
        {
          ParseTag(id, body, target);
        }
      }
    }

  private:
    static void ParseTag(uint32_t id, Binary::View data, MetaBuilder& target)
    {
      // http://id3.org/id3v2.3.0#Text_information_frames
      StringView encodedString(data.As<char>() + 1, data.Size() - 1);
      switch (id)
      {
      case 0x54495432:  //'TIT2'
        target.SetTitle(Strings::Sanitize(encodedString));
        break;
      case 0x54504531:  //'TPE1'
      case 0x544f5045:  //'TOPE'
        target.SetAuthor(Strings::Sanitize(encodedString));
        break;
      case 0x434f4d4d:  //'COMM'
        // http://id3.org/id3v2.3.0#Comments
        encodedString = encodedString.substr(3);
        [[fallthrough]];
      case 0x54585858:  //'TXXX'
      {
        // http://id3.org/id3v2.3.0#User_defined_text_information_frame
        const auto zeroPos = encodedString.find('\x00');
        Strings::Array strings;
        if (zeroPos != StringView::npos)
        {
          auto descr = Strings::Sanitize(encodedString.substr(0, zeroPos));
          if (!descr.empty())
          {
            strings.emplace_back(std::move(descr));
          }
          auto value = Strings::Sanitize(encodedString.substr(zeroPos + 1));
          if (!value.empty())
          {
            strings.emplace_back(std::move(value));
          }
        }
        else
        {
          auto val = Strings::Sanitize(encodedString);
          if (!val.empty())
          {
            strings.emplace_back(std::move(val));
          }
        }
        if (!strings.empty())
        {
          target.SetStrings(strings);
        }
      }
      break;
      case 0x54535345:  //'TSSE'
      case 0x54454e43:  //'TENC'
        target.SetProgram(Strings::Sanitize(encodedString));
        break;
      case 0x41504943:  // 'APIC'
      {
        Binary::DataInputStream input(data);
        input.ReadByte();                // encoding
        input.ReadCString(data.Size());  // mime
        input.ReadByte();                // type
        input.ReadCString(data.Size());  // description
        target.SetPicture(input.ReadRestData());
      }
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

  String MakeCompositeString(const EnhancedTagString& part1, const TagString& part2)
  {
    std::array<char, 90> buf;
    std::copy(part2.begin(), part2.end(), std::copy(part1.begin(), part1.end(), buf.begin()));
    return Strings::Sanitize(MakeStringView(buf));
  }

  // https://en.wikipedia.org/wiki/ID3#ID3v1_and_ID3v1.1
  bool ParseV1(Binary::DataInputStream& stream, MetaBuilder& target)
  {
    try
    {
      const Tag* tag = safe_ptr_cast<const Tag*>(stream.ReadData(sizeof(Tag)).Start());
      const EnhancedTag* enhancedTag = nullptr;
      if (tag->Title[0] == '+')
      {
        stream.Skip(sizeof(EnhancedTag));
        enhancedTag = safe_ptr_cast<const EnhancedTag*>(tag);
        tag = safe_ptr_cast<const Tag*>(enhancedTag + 1);
      }
      if (enhancedTag)
      {
        target.SetTitle(MakeCompositeString(enhancedTag->Title, tag->Title));
        target.SetAuthor(MakeCompositeString(enhancedTag->Artist, tag->Artist));
      }
      else
      {
        target.SetTitle(Strings::Sanitize(MakeStringView(tag->Title)));
        target.SetAuthor(Strings::Sanitize(MakeStringView(tag->Artist)));
      }
      // TODO: add MetaBuilder::SetComment field
      {
        const auto comment = MakeStringView(tag->Comment);
        const auto hasTrackNum = comment[28] == 0 || comment[28] == 0xff;  // standard violation
        target.SetStrings({Strings::Sanitize(hasTrackNum ? comment.substr(0, 28) : comment)});
      }
      return true;
    }
    catch (const std::exception&)
    {}
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

  bool ParseV2(Binary::DataInputStream& stream, MetaBuilder& target)
  {
    const auto& header = stream.Read<Header>();
    const uint_t tagSize = ((header.Size[0] & 0x7f) << 21) | ((header.Size[1] & 0x7f) << 14)
                           | ((header.Size[2] & 0x7f) << 7) | (header.Size[3] & 0x7f);
    const auto content = stream.ReadData(tagSize);
    const bool hasExtendedHeader = header.Flags & 0x40;
    try
    {
      if (header.Major >= 3 && !hasExtendedHeader)
      {
        V2Format(content).Parse(target);
      }
    }
    catch (const std::exception&)
    {}
    return true;
  }

  bool Parse(Binary::DataInputStream& stream, MetaBuilder& target)
  {
    static const std::size_t SIGNATURE_SIZE = 3;
    static const uint8_t V1_SIGNATURE[] = {'T', 'A', 'G'};
    static const uint8_t V2_SIGNATURE[] = {'I', 'D', '3'};
    if (stream.GetRestSize() < SIGNATURE_SIZE)
    {
      return false;
    }
    const auto* const sign = stream.PeekRawData(SIGNATURE_SIZE);
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
}  // namespace Formats::Chiptune::Id3
