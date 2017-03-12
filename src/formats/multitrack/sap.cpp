/**
* 
* @file
*
* @brief  SAP support implementation
*
* @author vitamin.caig@gmail.com
*
**/

//common includes
#include <byteorder.h>
#include <contract.h>
#include <crc.h>
#include <make_ptr.h>
#include <pointers.h>
//library includes
#include <binary/container_factories.h>
#include <binary/data_builder.h>
#include <binary/format_factories.h>
#include <binary/input_stream.h>
#include <formats/multitrack.h>
#include <parameters/convert.h>
#include <strings/array.h>
//std includes
#include <array>
#include <map>
#include <utility>

namespace Formats
{
namespace Multitrack
{
  namespace SAP
  {
    const std::size_t MAX_SIZE = 1048576;
    
    const std::string FORMAT =
      "'S'A'P"
      "0d0a"
      "'A|'N|'D|'S|'D|'S|'N|'T|'F|'I|'M|'P|'C|'T"
      "'U|'A|'A|'O|'E|'T|'T|'Y|'A|'N|'U|'L|'O|'I"
      "'T|'M|'T|'N|'F|'E|'S|'P|'S|'I|'S|'A|'V|'M"
      "'H|'E|'E|'G|'S|'R|'C|'E|'T|'T|'I|'Y|'O|'E"
      "'O|' |' |'S|'O|'E|' |' |'P|' |'C|'E|'X|' "
     ;
     
    typedef std::array<uint8_t, 5> TextSignatureType;

    const TextSignatureType TEXT_SIGNATURE = {{'S', 'A', 'P', 0x0d, 0x0a}};
    const std::string SONGS = "SONGS";
    const std::string DEFSONG = "DEFSONG";
    
    typedef std::array<uint8_t, 2> BinarySignatureType;
    const BinarySignatureType BINARY_SIGNATURE = {{0xff, 0xff}};
     
    const std::size_t MIN_SIZE = 256;
    
    class Builder
    {
    public:
      virtual ~Builder() = default;

      virtual void SetProperty(const String& name, const String& value) = 0;
      virtual void SetBlock(const uint_t start, const uint8_t* data, std::size_t size) = 0;
    };
    
    class DataBuilder : public Builder
    {
    public:
      typedef std::shared_ptr<const DataBuilder> Ptr;
      typedef std::shared_ptr<DataBuilder> RWPtr;
      
      DataBuilder()
        : TracksCount(1)
        , DefaultTrack(0)
      {
      }
      
      void SetProperty(const String& name, const String& value) override
      {
        if (name == DEFSONG)
        {
          Require(Parameters::ConvertFromString(value, DefaultTrack));
          return;
        }
        else if (name == SONGS)
        {
          Require(Parameters::ConvertFromString(value, TracksCount));
        }
        Lines.push_back(name + " " + value);
      }
      
      void SetBlock(const uint_t start, const uint8_t* data, std::size_t size) override
      {
        Blocks[start].assign(data, data + size);
      }
      
      uint_t GetTracksCount() const
      {
        return TracksCount;
      }
      
      uint_t GetStartTrack() const
      {
        return DefaultTrack;
      }
      
      uint_t GetFixedCrc(uint_t startTrack) const
      {
        uint32_t crc = startTrack;
        for (const auto& blk : Blocks)
        {
          crc = Crc32(&blk.second.front(), blk.second.size(), crc);
        }
        return crc;
      }
      
      Binary::Container::Ptr Rebuild(uint_t startTrack) const
      {
        Require(TracksCount != 1);
        Binary::DataBuilder builder;
        builder.Add(TEXT_SIGNATURE);
        DumpTextPart(builder);
        AddString(DEFSONG + " " + Parameters::ConvertToString(startTrack), builder);
        builder.Add(BINARY_SIGNATURE);
        DumpBinaryPart(builder);
        return builder.CaptureResult();
      }
    private:
      void DumpTextPart(Binary::DataBuilder& builder) const
      {
        for (const auto& line : Lines)
        {
          AddString(line, builder);
        }
      }
      
      static void AddString(const String& str, Binary::DataBuilder& builder)
      {
        uint8_t* const dst = static_cast<uint8_t*>(builder.Allocate(str.size() + 2));
        uint8_t* const end = std::copy(str.begin(), str.end(), dst);
        end[0] = 0x0d;
        end[1] = 0x0a;
      }
      
      void DumpBinaryPart(Binary::DataBuilder& builder) const
      {
        for (const auto& blk : Blocks)
        {
          const uint_t addr = blk.first;
          const std::size_t size = blk.second.size();
          builder.Add(fromLE<uint16_t>(addr));
          builder.Add(fromLE<uint16_t>(addr + size - 1));
          uint8_t* const dst = static_cast<uint8_t*>(builder.Allocate(size));
          std::copy(blk.second.begin(), blk.second.end(), dst);
        }
      }
    private:
      Strings::Array Lines;
      Parameters::IntType TracksCount;
      Parameters::IntType DefaultTrack;
      std::map<uint_t, Dump> Blocks;
    };
    
    class Container : public Formats::Multitrack::Container
    {
    public:
      Container(DataBuilder::Ptr content, Binary::Container::Ptr delegate, uint_t startTrack)
        : Content(std::move(content))
        , Delegate(std::move(delegate))
        , StartTrack(startTrack)
      {
      }
      
      //Binary::Container
      const void* Start() const override
      {
        return Delegate->Start();
      }

      std::size_t Size() const override
      {
        return Delegate->Size();
      }

      Binary::Container::Ptr GetSubcontainer(std::size_t offset, std::size_t size) const override
      {
        return Delegate->GetSubcontainer(offset, size);
      }
      
      //Formats::Multitrack::Container
      uint_t FixedChecksum() const override
      {
        return Content->GetFixedCrc(StartTrack);
      }

      uint_t TracksCount() const override
      {
        return Content->GetTracksCount();
      }

      uint_t StartTrackIndex() const override
      {
        return StartTrack;
      }
      
      Container::Ptr WithStartTrackIndex(uint_t idx) const override
      {
        return MakePtr<Container>(Content, Content->Rebuild(idx), idx);
      }
    private:
      const DataBuilder::Ptr Content;
      const Binary::Container::Ptr Delegate;
      const uint_t StartTrack;
    };

    class Decoder : public Formats::Multitrack::Decoder
    {
    public:
      //Use match only due to lack of end detection
      Decoder()
        : Format(Binary::CreateMatchOnlyFormat(FORMAT, MIN_SIZE))
      {
      }

      Binary::Format::Ptr GetFormat() const override
      {
        return Format;
      }

      bool Check(const Binary::Container& rawData) const override
      {
        return Format->Match(rawData);
      }

      Formats::Multitrack::Container::Ptr Decode(const Binary::Container& rawData) const override
      {
        try
        {
          const DataBuilder::RWPtr builder = MakeRWPtr<DataBuilder>();
          const Binary::Container::Ptr data = Parse(rawData, *builder);
          return MakePtr<Container>(builder, data, builder->GetStartTrack());
        }
        catch (const std::exception&)
        {
        }
        return Formats::Multitrack::Container::Ptr();
      }
    private:
      static Binary::Container::Ptr Parse(const Binary::Container& rawData, Builder& builder)
      {
        Binary::InputStream stream(rawData);
        Require(stream.ReadField<TextSignatureType>() == TEXT_SIGNATURE);
        ParseTextPart(stream, builder);
        ParseBinaryPart(stream, builder);
        return stream.GetReadData();
      }

      static void ParseTextPart(Binary::InputStream& stream, Builder& builder)
      {
        for (;;)
        {
          const BinarySignatureType binSig = stream.ReadField<BinarySignatureType>();
          if (binSig == BINARY_SIGNATURE)
          {
            break;
          }
          String name, value;
          const std::string line = std::string(binSig.begin(), binSig.end()) + stream.ReadString();
          std::string::size_type spacePos = line.find_first_of(' ');
          if (spacePos != std::string::npos)
          {
            name = line.substr(0, spacePos);
            while (line[spacePos] == ' ')
            {
              ++spacePos;
            }
            value = line.substr(spacePos);
          }
          else
          {
            name = line;
          }
          builder.SetProperty(name, value);
        }
      }
      
      static void ParseBinaryPart(Binary::InputStream& stream, Builder& builder)
      {
        while (stream.GetRestSize())
        {
          const uint_t first = fromLE(stream.ReadField<uint16_t>());
          if ((first & 0xff) == BINARY_SIGNATURE[0] &&
              (first >> 8) == BINARY_SIGNATURE[1])
          {
            //skip possible headers inside
            continue;
          }
          const uint_t last = fromLE(stream.ReadField<uint16_t>());
          Require(first <= last);
          const std::size_t size = last + 1 - first;
          const auto data = stream.ReadRawData(size);
          builder.SetBlock(first, data, size);
        }
      }
    private:
      const Binary::Format::Ptr Format;
    };
  }//namespace SAP

  Decoder::Ptr CreateSAPDecoder()
  {
    return MakePtr<SAP::Decoder>();
  }
}//namespace Multitrack
}//namespace Formats
