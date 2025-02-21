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

#include "strings/src/utf8.h"

#include "strings/encoding.h"
#include "strings/sanitize.h"

#include "string_view.h"

#include <array>

namespace Formats::Chiptune::Id3
{
  constexpr auto operator""_tag(const char* tag, std::size_t)
  {
    return (uint_t(tag[0]) << 24) | (uint_t(tag[1]) << 16) | (uint_t(tag[2]) << 8) | tag[3];
  }

  namespace Tags
  {
    // 0 - not found, else priority (higher the better)
    template<class T>
    uint_t FindIn(const T& tags, uint32_t tag)
    {
      const auto end = std::end(tags);
      return end - std::find(std::begin(tags), end, tag);
    }

    constexpr uint32_t FOR_TITLE[] = {
        "TIT2"_tag,
        "TIT3"_tag,
    };

    constexpr uint32_t FOR_AUTHOR[] = {
        "TPE1"_tag,
        "TOPE"_tag,
        "TCOM"_tag,
    };

    constexpr uint32_t FOR_PROGRAM[] = {
        "TSSE"_tag,
    };

    constexpr uint32_t FOR_COMMENT[] = {
        "COMM"_tag,
        "TALB"_tag,
        "TOAL"_tag,
    };
  }  // namespace Tags

  class PrioritizedString
  {
  public:
    void Set(uint_t prio, String value)
    {
      if (prio > Prio)
      {
        Prio = prio;
        Value = std::move(value);
      }
    }

    const String* Get() const
    {
      return Prio > 0 ? &Value : nullptr;
    }

  private:
    uint_t Prio = 0;
    String Value;
  };

  class TagMetaAdapter
  {
  public:
    explicit TagMetaAdapter(MetaBuilder& delegate)
      : Delegate(delegate)
    {}

    void SetTag(uint32_t tag, Binary::View data)
    {
      if (const auto asTitle = Tags::FindIn(Tags::FOR_TITLE, tag))
      {
        Title.Set(asTitle, GetString(data));
      }
      else if (const auto asAuthor = Tags::FindIn(Tags::FOR_AUTHOR, tag))
      {
        Author.Set(asAuthor, GetString(data));
      }
      else if (const auto asProgram = Tags::FindIn(Tags::FOR_PROGRAM, tag))
      {
        Program.Set(asProgram, GetString(data));
      }
      else if (const auto asComment = Tags::FindIn(Tags::FOR_COMMENT, tag))
      {
        Comment.Set(asComment, tag == "COMM"_tag ? ParseText(data.SubView(4)).second : GetString(data));
      }
      else if (tag == "TXXX"_tag)
      {
        const auto& text = ParseText(data.SubView(1));
        Strings.emplace_back(text.first + ": " + text.second);
      }
      else if (tag == "APIC"_tag)
      {
        SetPicture(data);
      }
    }

    void Apply()
    {
      if (const auto* val = Title.Get())
      {
        Delegate.SetTitle(*val);
      }
      if (const auto* val = Author.Get())
      {
        Delegate.SetAuthor(*val);
      }
      if (const auto* val = Program.Get())
      {
        Delegate.SetProgram(*val);
      }
      if (const auto* val = Comment.Get())
      {
        Delegate.SetComment(*val);
      }
      if (!Strings.empty())
      {
        Delegate.SetStrings(Strings);
      }
    }

  private:
    static String GetString(Binary::View data)
    {
      return Strings::Sanitize({data.As<char>() + 1, data.Size() - 1});
    }

    static std::pair<String, String> ParseText(Binary::View data)
    {
      StringView encodedString(data.As<char>(), data.Size());
      String decodedStorage;
      if (Strings::IsUtf16(encodedString))
      {
        decodedStorage = Strings::ToAutoUtf8(encodedString);
        encodedString = decodedStorage;
      }
      // http://id3.org/id3v2.3.0#User_defined_text_information_frame
      const auto zeroPos = encodedString.find('\x00');
      if (zeroPos != StringView::npos)
      {
        return std::make_pair(Strings::Sanitize(encodedString.substr(0, zeroPos)),
                              Strings::Sanitize(encodedString.substr(zeroPos + 1)));
      }
      return std::make_pair(String(), Strings::Sanitize(encodedString));
    }

    void SetPicture(Binary::View data)
    {
      Binary::DataInputStream input(data);
      input.ReadByte();                // encoding
      input.ReadCString(data.Size());  // mime
      input.ReadByte();                // type
      input.ReadCString(data.Size());  // description
      Delegate.SetPicture(input.ReadRestData());
    }

  private:
    MetaBuilder& Delegate;
    PrioritizedString Title;
    PrioritizedString Author;
    PrioritizedString Program;
    PrioritizedString Comment;
    Strings::Array Strings;
  };
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
      TagMetaAdapter out(target);
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
          out.SetTag(id, body);
        }
      }
      out.Apply();
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
      {
        const auto comment = MakeStringView(tag->Comment);
        const auto hasTrackNum = comment[28] == 0 || comment[28] == 0xff;  // standard violation
        target.SetComment(Strings::Sanitize(hasTrackNum ? comment.substr(0, 28) : comment));
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
