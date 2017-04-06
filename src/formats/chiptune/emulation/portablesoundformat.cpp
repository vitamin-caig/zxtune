/**
* 
* @file
*
* @brief  Portable Sound Format family support implementation
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "portablesoundformat.h"
//common includes
#include <byteorder.h>
#include <crc.h>
#include <make_ptr.h>
//library includes
#include <binary/compression/zlib_container.h>
#include <binary/data_builder.h>
#include <binary/input_stream.h>
#include <debug/log.h>
#include <formats/chiptune/container.h>
#include <math/numeric.h>
#include <strings/encoding.h>
#include <strings/prefixed_index.h>
#include <strings/trim.h>
#include <time/duration.h>
//std includes
#include <cctype>
#include <set>

namespace Formats
{
namespace Chiptune
{
namespace PortableSoundFormat
{
  const Debug::Stream Dbg("Formats::Chiptune::PortableSoundFormat");

  const uint8_t SIGNATURE[] = {'P', 'S', 'F'};
  
  namespace Tags
  {
    const uint8_t SIGNATURE[] = {'[', 'T', 'A', 'G', ']'};
    const String LIB_PREFIX = "_lib";
    
    const char UTF8[] = "utf8";
    const char TITLE[] = "title";
    const char ARTIST[] = "artist";
    const char GAME[] = "game";
    const char YEAR[] = "year";
    const char GENRE[] = "genre";
    const char COMMENT[] = "comment";
    const char COPYRIGHT[] = "copyright";
    const char XSFBY_SUFFIX[] = "sfby";
    const char LENGTH[] = "length";
    const char FADE[] = "fade";
    const char VOLUME[] = "volume";
    
    static String MakeName(StringView str)
    {
      String res;
      res.reserve(str.size());
      for (const auto sym : str)
      {
        res += std::tolower(sym);
      }
      return res;
    }
  }
  
  class Format
  {
  public:
    explicit Format(const Binary::Container& data)
      : Stream(data)
    {
    }
    
    Container::Ptr Parse(Builder& target)
    {
      ParseSignature(target);
      const auto dataStart = Stream.GetPosition();
      ParseData(target);
      const auto dataEnd = Stream.GetPosition();
      ParseTags(target);
      return CreateCalculatingCrcContainer(Stream.GetReadData(), dataStart, dataEnd - dataStart);
    }
  private:
    void ParseSignature(Builder& target)
    {
      const auto sign = Stream.ReadRawData(sizeof(SIGNATURE));
      Require(0 == std::memcmp(sign, SIGNATURE, sizeof(SIGNATURE)));
      const uint_t version = Stream.ReadField<uint8_t>();
      target.SetVersion(version);
      Dbg("Version %1%", version);
    }
    
    void ParseData(Builder& target)
    {
      const auto reservedSize = fromLE(Stream.ReadField<uint32_t>());
      const auto compressedSize = fromLE(Stream.ReadField<uint32_t>());
      const auto compressedCrc = fromLE(Stream.ReadField<uint32_t>());
      if (auto reserved = Stream.ReadData(reservedSize))
      {
        Dbg("Reserved section %1% bytes", reservedSize);
        target.SetReservedSection(std::move(reserved));
      }
      if (compressedSize)
      {
        //program section is 4-byte aligned
        Stream.Seek(Math::Align<uint32_t>(Stream.GetPosition(), 4));
        auto programPacked = Stream.ReadData(compressedSize);
        CheckCrc(*programPacked, compressedCrc);
        Dbg("Program section %1% bytes", compressedSize);
        auto programUnpacked = Binary::Compression::Zlib::CreateDeferredDecompressContainer(std::move(programPacked));
        target.SetProgramSection(std::move(programUnpacked));
      }
    }
    
    static void CheckCrc(const Binary::Data& blob, uint32_t crc)
    {
      Require(crc == Crc32(static_cast<const uint8_t*>(blob.Start()), blob.Size()));
    }
    
    void ParseTags(Builder& target)
    {
      if (!ReadTagSignature())
      {
        return;
      };
      bool utf8 = false;
      String comment;
      while (Stream.GetRestSize())
      {
        String name;
        StringView valueView;
        if (!ReadTagVariable(name, valueView))
        {
          //Blank lines, or lines not of the form "variable=value", are ignored.
          continue;
        }
        Dbg("tags[%1%]=%2%", name, valueView);
        if (const auto num = FindLibraryNumber(name))
        {
          target.SetLibrary(num, valueView.to_string());
          continue;
        }
        else if (name == Tags::UTF8)
        {
          utf8 = true;
          continue;
        }
        const auto value = utf8
          ? valueView.to_string()
          : Strings::ToAutoUtf8(valueView);
        if (name == Tags::TITLE)
        {
          target.SetTitle(value);
        }
        else if (name == Tags::ARTIST)
        {
          target.SetArtist(value);
        }
        else if (name == Tags::GAME)
        {
          target.SetGame(value);
        }
        else if (name == Tags::YEAR)
        {
          target.SetYear(value);
        }
        else if (name == Tags::GENRE)
        {
          target.SetGenre(value);
        }
        else if (name == Tags::COMMENT)
        {
          if (comment.empty())
          {
            comment = value;
          }
          else
          {
            comment += '\n';
            comment += value;
          }
        }
        else if (name == Tags::COPYRIGHT)
        {
          target.SetCopyright(value);
        }
        else if (name.npos != name.find(Tags::XSFBY_SUFFIX))
        {
          target.SetDumper(value);
        }
        else if (name == Tags::LENGTH)
        {
          target.SetLength(ParseTime(value));
        }
        else if (name == Tags::FADE)
        {
          target.SetFade(ParseTime(value));
        }
        else if (name == Tags::VOLUME)
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
        target.SetComment(std::move(comment));
      }
    }
    
    bool ReadTagSignature()
    {
      if (Stream.GetRestSize() < sizeof(Tags::SIGNATURE))
      {
        return false;
      }
      const auto currentPosition = Stream.GetPosition();
      const auto sign = Stream.ReadRawData(sizeof(Tags::SIGNATURE));
      if (0 == std::memcmp(sign, Tags::SIGNATURE, sizeof(Tags::SIGNATURE)))
      {
        return true;
      }
      Stream.Seek(currentPosition);
      return false;
    }
    
    bool ReadTagVariable(String& name, StringView& value)
    {
      const auto line = Stream.ReadString();
      const auto eqPos = line.find('=');
      if (eqPos != line.npos)
      {
        name = Tags::MakeName(Strings::TrimSpaces(line.substr(0, eqPos)));
        value = Strings::TrimSpaces(line.substr(eqPos + 1));
        return true;
      }
      else
      {
        return false;
      }
    }
    
    static uint_t FindLibraryNumber(StringView tagName)
    {
      if (tagName == Tags::LIB_PREFIX)
      {
        return 1;
      }
      else
      {
        const Strings::PrefixedIndex lib(Tags::LIB_PREFIX, tagName);
        const auto num = lib.IsValid() ? lib.GetIndex() : 0;
        Require(num != 1);
        return num;
      }
    }

    static Time::Milliseconds ParseTime(String val)
    {
      //[[hours:]minutes:]seconds.decimal
      Time::Milliseconds result;
      String::size_type start = 0;
      auto end = val.find_first_of(':', start);
      while (end != String::npos)
      {
        val[end] = 0;
        result = Time::Milliseconds(result.Get() * 60 + result.PER_SECOND * std::atoi(val.c_str()  + start));
        end = val.find_first_of(':', start = end + 1);
      }
      return Time::Milliseconds(result.Get() * 60 + result.PER_SECOND * std::atof(val.c_str() + start));
    }
    
    static float ParseVolume(String val)
    {
      return std::atof(val.c_str());
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
      return Container::Ptr();
    }
  }

  /*
  class PackedBlobBuilder : public BlobBuilder
  {
  public:
    PackedBlobBuilder()
      : Version(0)
    {
    }
    
    void SetVersion(uint_t ver) override
    {
      Version = ver;
    }

    void SetReservedSection(Binary::Container::Ptr blob) override
    {
      Reserved = std::move(blob);
    }
    
    void SetProgramSection(Binary::Container::Ptr blob) override
    {
      Program = Binary::Compression::Zlib::Compress(blob->Start(), blob->Size());
    }
    
    void SetTitle(String title) override
    {
      SetTag(Tags::TITLE, std::move(title));
    }
    
    void SetArtist(String artist) override
    {
      SetTag(Tags::ARTIST, std::move(artist));
    }
    
    void SetGame(String game) override
    {
      SetTag(Tags::GAME, std::move(game));
    }
    
    void SetYear(String date)
    {
      SetTag(Tags::YEAR, std::move(date));
    }
    
    void SetGenre(String genre)
    {
      SetTag(Tags::GENRE, std::move(genre));
    }
    
    void SetComment(String comment)
    {
      SetTag(Tags::COMMENT, std::move(comment));
    }
    
    void SetCopyright(String copyright)
    {
      SetTag(Tags::COPYRIGHT, std::move(copyright));
    }
    
    void SetDumper(String dumper)
    {
      //TODO:
    }

    void SetLength(Time::Milliseconds duration)
    {
      SetTag(Tags::LENGTH, ToString(duration));
    }
    
    void SetFade(Time::Milliseconds duration)
    {
      SetTag(Tags::FADE, ToString(duration));
    }
    
    void SetTag(String name, String value) override
    {
      if (Tags.empty())
      {
        Tags.emplace_back(Tags::UTF8, "1");
      }
      Require(name == Tags::COMMENT || TagNames.insert(name).second);
      Tags.emplace_back(name, value);
    }
    
    void SetLibrary(uint_t num, String filename) override
    {
      Require(num > 0);
      SetTag(num == 1 ? Tags::LIB_PREFIX : Strings::PrefixedIndex(Tags::LIB_PREFIX, num).ToString(), std::move(filename));
    }

    Binary::Container::Ptr GetResult() const
    {
      Require(Version != 0);
      //Require(!Tags.empty());
      PSFBuilder result;
      result.AddSignature(Version);
      if (Reserved)
      {
        result.AddBlobs(Reserved->Start(), Reserved->Size(), Program.data(), Program.size());
      }
      else
      {
        result.AddBlobs(nullptr, 0, Program.data(), Program.size());
      }
      for (const auto& tag : Tags)
      {
        result.AddTag(tag.first, tag.second);
      }
      return result.CaptureResult();
    }
  private:
    static String ToString(Time::Milliseconds stamp)
    {
      return Time::MillisecondsDuration(stamp.Get(), Time::Milliseconds(1)).ToString();
    }
  
    class PSFBuilder
    {
    public:
      void AddSignature(uint_t version)
      {
        Delegate.Add(SIGNATURE, sizeof(SIGNATURE));
        Delegate.Add<uint8_t>(version);
      }
      
      void AddBlobs(const void* resData, std::size_t resSize, const void* prgData, std::size_t prgSize)
      {
        const uint32_t prgCrc = prgSize ? Crc32(static_cast<const uint8_t*>(prgData), prgSize) : 0;
        Delegate.Add(fromLE<uint32_t>(resSize));
        Delegate.Add(fromLE<uint32_t>(prgSize));
        Delegate.Add(fromLE<uint32_t>(prgCrc));
        if (resSize)
        {
          Delegate.Add(resData, resSize);
        }
        if (prgSize)
        {
          Delegate.Resize(Math::Align<std::size_t>(Delegate.Size(), 4));
          Delegate.Add(prgData, prgSize);
        }
      }
      
      void AddTag(const String& name, const String& value)
      {
        AddTagsSignature();
        const auto size = name.size() + 1 + value.size() + 1;
        auto dst = static_cast<char*>(Delegate.Allocate(size));
        dst = std::copy(name.begin(), name.end(), dst);
        *dst++ = '=';
        dst = std::copy(value.begin(), value.end(), dst);
        *dst = '\x0a';
      }
      
      Binary::Container::Ptr CaptureResult()
      {
        return Delegate.CaptureResult();
      }
    private:
      void AddTagsSignature()
      {
        if (!HasTagsSignature)
        {
          Delegate.Add(TAG_SIGNATURE, sizeof(TAG_SIGNATURE));
          HasTagsSignature = true;
        }
      }
    private:
      Binary::DataBuilder Delegate;
      bool HasTagsSignature = false;
    };
  private:
    uint_t Version;
    Binary::Data::Ptr Reserved;
    Dump Program;
    std::set<String> TagNames;
    std::vector<std::pair<String, String>> Tags;
  };
      
  BlobBuilder::Ptr CreateBlobBuilder()
  {
    return MakePtr<PackedBlobBuilder>();
  }
  */
}
}
}
