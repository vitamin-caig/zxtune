/**
 *
 * @file
 *
 * @brief  Portable Sound Format family support implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include "formats/chiptune/emulation/portablesoundformat.h"
// common includes
#include <byteorder.h>
#include <make_ptr.h>
// library includes
#include <binary/crc.h>
#include <binary/data_builder.h>
#include <binary/input_stream.h>
#include <debug/log.h>
#include <formats/chiptune/container.h>
#include <strings/casing.h>
#include <strings/prefixed_index.h>
#include <strings/sanitize.h>
#include <strings/trim.h>
#include <time/duration.h>
// std includes
#include <set>

namespace Formats::Chiptune::PortableSoundFormat
{
  const Debug::Stream Dbg("Formats::Chiptune::PortableSoundFormat");

  const uint8_t SIGNATURE[] = {'P', 'S', 'F'};

  namespace Tags
  {
    const uint8_t SIGNATURE[] = {'[', 'T', 'A', 'G', ']'};
    const auto LIB_PREFIX = "_lib"_sv;

    const auto UTF8 = "utf8"_sv;
    const auto TITLE = "title"_sv;
    const auto ARTIST = "artist"_sv;
    const auto GAME = "game"_sv;
    const auto YEAR = "year"_sv;
    const auto GENRE = "genre"_sv;
    const auto COMMENT = "comment"_sv;
    const auto COPYRIGHT = "copyright"_sv;
    const auto XSFBY_SUFFIX = "sfby"_sv;
    const auto LENGTH = "length"_sv;
    const auto FADE = "fade"_sv;
    const auto VOLUME = "volume"_sv;

    bool Match(StringView str, StringView tag)
    {
      return Strings::EqualNoCaseAscii(str, tag);
    }
  }  // namespace Tags

  class Format
  {
  public:
    explicit Format(const Binary::Container& data)
      : Stream(data)
    {}

    Container::Ptr Parse(Builder& target)
    {
      ParseSignature(target);
      const auto dataStart = Stream.GetPosition();
      ParseData(target);
      const auto dataEnd = Stream.GetPosition();
      ParseTags(target);
      return CreateCalculatingCrcContainer(Stream.GetReadContainer(), dataStart, dataEnd - dataStart);
    }

  private:
    void ParseSignature(Builder& target)
    {
      const auto sign = Stream.ReadData(sizeof(SIGNATURE));
      Require(0 == std::memcmp(sign.Start(), SIGNATURE, sizeof(SIGNATURE)));
      const uint_t version = Stream.ReadByte();
      target.SetVersion(version);
      Dbg("Version {}", version);
    }

    void ParseData(Builder& target)
    {
      const std::size_t reservedSize = Stream.Read<le_uint32_t>();
      const std::size_t compressedSize = Stream.Read<le_uint32_t>();
      const uint32_t compressedCrc = Stream.Read<le_uint32_t>();
      if (auto reserved = Stream.ReadContainer(reservedSize))
      {
        Dbg("Reserved section {} bytes", reservedSize);
        target.SetReservedSection(std::move(reserved));
      }
      if (compressedSize)
      {
        auto programPacked = Stream.ReadContainer(compressedSize);
        Require(compressedCrc == Binary::Crc32(*programPacked));
        Dbg("Program section {} bytes", compressedSize);
        target.SetPackedProgramSection(std::move(programPacked));
      }
    }

    class TagsParser
    {
    public:
      bool Parse(StringView line)
      {
        const auto eqPos = line.find('=');
        if (eqPos != line.npos)
        {
          Name = Strings::TrimSpaces(line.substr(0, eqPos));
          Value = line.substr(eqPos + 1);
          return true;
        }
        else
        {
          return false;
        }
      }

      StringView GetName() const
      {
        return Name;
      }

      StringView GetValue() const
      {
        return Value;
      }

    private:
      StringView Name;
      StringView Value;
    };

    void ParseTags(Builder& target)
    {
      if (!ReadTagSignature())
      {
        return;
      };
      TagsParser tag;
      String comment;
      auto& meta = target.GetMetaBuilder();
      while (Stream.GetRestSize())
      {
        if (!tag.Parse(Stream.ReadString()))
        {
          // Blank lines, or lines not of the form "variable=value", are ignored.
          continue;
        }
        const auto name = tag.GetName();
        const auto value = Strings::Sanitize(tag.GetValue());
        Dbg("tags[{}]={}", name, value);
        if (const auto num = FindLibraryNumber(name))
        {
          target.SetLibrary(num, value);
          continue;
        }
        else if (name == Tags::UTF8)
        {
          continue;
        }
        if (Tags::Match(name, Tags::TITLE))
        {
          meta.SetTitle(value);
        }
        else if (Tags::Match(name, Tags::ARTIST))
        {
          meta.SetAuthor(value);
        }
        else if (Tags::Match(name, Tags::GAME))
        {
          meta.SetProgram(value);
        }
        else if (Tags::Match(name, Tags::YEAR))
        {
          target.SetYear(value);
        }
        else if (Tags::Match(name, Tags::GENRE))
        {
          target.SetGenre(value);
        }
        else if (Tags::Match(name, Tags::COMMENT))
        {
          if (!comment.empty())
          {
            comment += '\n';
          }
          comment.append(value);
        }
        else if (Tags::Match(name, Tags::COPYRIGHT))
        {
          target.SetCopyright(value);
        }
        else if (name.npos != name.find(Tags::XSFBY_SUFFIX))
        {
          target.SetDumper(value);
        }
        else if (Tags::Match(name, Tags::LENGTH))
        {
          target.SetLength(ParseTime(value));
        }
        else if (Tags::Match(name, Tags::FADE))
        {
          target.SetFade(ParseTime(value));
        }
        else if (Tags::Match(name, Tags::VOLUME))
        {
          target.SetVolume(ParseVolume(value));
        }
        else
        {
          target.SetTag(name, value);
        }
      }
      if (!comment.empty())
      {
        meta.SetComment(comment);
      }
    }

    bool ReadTagSignature()
    {
      if (Stream.GetRestSize() < sizeof(Tags::SIGNATURE))
      {
        return false;
      }
      const auto currentPosition = Stream.GetPosition();
      const auto sign = Stream.ReadData(sizeof(Tags::SIGNATURE));
      if (0 == std::memcmp(sign.Start(), Tags::SIGNATURE, sizeof(Tags::SIGNATURE)))
      {
        return true;
      }
      Stream.Seek(currentPosition);
      return false;
    }

    static uint_t FindLibraryNumber(StringView tagName)
    {
      if (Tags::Match(tagName, Tags::LIB_PREFIX))
      {
        return 1;
      }
      else
      {
        const auto lib = Strings::PrefixedIndex::ParseNoCase(Tags::LIB_PREFIX, tagName);
        const auto num = lib.IsValid() ? lib.GetIndex() : 0;
        Require(num != 1);
        return num;
      }
    }

    static Time::Milliseconds ParseTime(String val)
    {
      //[[hours:]minutes:]seconds.decimal
      uint_t result = 0;
      String::size_type start = 0;
      auto end = val.find_first_of(':', start);
      while (end != String::npos)
      {
        val[end] = 0;
        result = result * 60 + 1000 * std::atoi(val.c_str() + start);
        end = val.find_first_of(':', start = end + 1);
      }
      return Time::Milliseconds{result * 60 + static_cast<uint_t>(1000 * std::atof(val.c_str() + start))};
    }

    static float ParseVolume(const String& val)
    {
      return static_cast<float>(std::atof(val.c_str()));
    }

  private:
    Binary::InputStream Stream;
  };

  Container::Ptr Parse(const Binary::Container& data, Builder& target)
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
}  // namespace Formats::Chiptune::PortableSoundFormat
