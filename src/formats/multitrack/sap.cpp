/**
 *
 * @file
 *
 * @brief  SAP support implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// common includes
#include <byteorder.h>
#include <contract.h>
#include <make_ptr.h>
#include <pointers.h>
// library includes
#include <binary/container_base.h>
#include <binary/crc.h>
#include <binary/data_builder.h>
#include <binary/format_factories.h>
#include <binary/input_stream.h>
#include <formats/multitrack.h>
#include <strings/array.h>
#include <strings/conversion.h>
// std includes
#include <array>
#include <map>
#include <utility>

namespace Formats::Multitrack
{
  namespace SAP
  {
    // const std::size_t MAX_SIZE = 1048576;

    const auto FORMAT =
        "'S'A'P"
        "0d0a"
        "'A|'N|'D|'S|'D|'S|'N|'T|'F|'I|'M|'P|'C|'T"
        "'U|'A|'A|'O|'E|'T|'T|'Y|'A|'N|'U|'L|'O|'I"
        "'T|'M|'T|'N|'F|'E|'S|'P|'S|'I|'S|'A|'V|'M"
        "'H|'E|'E|'G|'S|'R|'C|'E|'T|'T|'I|'Y|'O|'E"
        "'O|' |' |'S|'O|'E|' |' |'P|' |'C|'E|'X|' "
        ""_sv;

    const Char DESCRIPTION[] = "Slight Atari Player Sound Format";

    typedef std::array<uint8_t, 5> TextSignatureType;

    const TextSignatureType TEXT_SIGNATURE = {{'S', 'A', 'P', 0x0d, 0x0a}};
    const auto SONGS = "SONGS"_sv;
    const auto DEFSONG = "DEFSONG"_sv;

    typedef std::array<uint8_t, 2> BinarySignatureType;
    const BinarySignatureType BINARY_SIGNATURE = {{0xff, 0xff}};

    const std::size_t MIN_SIZE = 256;

    class Builder
    {
    public:
      virtual ~Builder() = default;

      virtual void SetProperty(StringView name, StringView value) = 0;
      virtual void SetBlock(const uint_t start, Binary::View data) = 0;
    };

    class DataBuilder : public Builder
    {
    public:
      typedef std::shared_ptr<const DataBuilder> Ptr;
      typedef std::shared_ptr<DataBuilder> RWPtr;

      DataBuilder() {}

      void SetProperty(StringView name, StringView value) override
      {
        if (name == DEFSONG)
        {
          Require(Strings::Parse(value, DefaultTrack));
          return;
        }
        else if (name == SONGS)
        {
          Require(Strings::Parse(value, TracksCount));
        }
        if (value.empty())
        {
          Lines.push_back(name.to_string());
        }
        else
        {
          Lines.emplace_back(name.to_string() + " " + value.to_string());
        }
      }

      void SetBlock(const uint_t start, Binary::View data) override
      {
        Blocks.emplace(start, data);
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
          crc = Binary::Crc32(blk.second, crc);
        }
        return crc;
      }

    private:
      Strings::Array Lines;
      uint_t TracksCount = 1;
      uint_t DefaultTrack = 0;
      std::map<uint_t, Binary::View> Blocks;
    };

    class Container : public Binary::BaseContainer<Multitrack::Container>
    {
    public:
      Container(DataBuilder::Ptr content, Binary::Container::Ptr delegate, uint_t startTrack)
        : BaseContainer(std::move(delegate))
        , Content(std::move(content))
        , StartTrack(startTrack)
      {}

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

    private:
      const DataBuilder::Ptr Content;
      const uint_t StartTrack;
    };

    class Decoder : public Formats::Multitrack::Decoder
    {
    public:
      // Use match only due to lack of end detection
      Decoder()
        : Format(Binary::CreateMatchOnlyFormat(FORMAT, MIN_SIZE))
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

      Formats::Multitrack::Container::Ptr Decode(const Binary::Container& rawData) const override
      {
        if (!Format->Match(rawData))
        {
          return {};
        }
        try
        {
          auto builder = MakeRWPtr<DataBuilder>();
          auto data = Parse(rawData, *builder);
          const auto startTrack = builder->GetStartTrack();
          return MakePtr<Container>(std::move(builder), std::move(data), startTrack);
        }
        catch (const std::exception&)
        {}
        return {};
      }

    private:
      static Binary::Container::Ptr Parse(const Binary::Container& rawData, Builder& builder)
      {
        Binary::InputStream stream(rawData);
        Require(stream.Read<TextSignatureType>() == TEXT_SIGNATURE);
        ParseTextPart(stream, builder);
        ParseBinaryPart(stream, builder);
        return stream.GetReadContainer();
      }

      static void ParseTextPart(Binary::DataInputStream& stream, Builder& builder)
      {
        for (;;)
        {
          const auto pos = stream.GetPosition();
          if (stream.Read<BinarySignatureType>() == BINARY_SIGNATURE)
          {
            break;
          }
          stream.Seek(pos);
          StringView name, value;
          const auto line = stream.ReadString();
          auto spacePos = line.find(' ');
          if (spacePos != line.npos)
          {
            name = line.substr(0, spacePos);
            while (line[spacePos] == ' ' && spacePos < line.size())
            {
              ++spacePos;
            }
            value = line.substr(spacePos);
          }
          else
          {
            name = line;
          }
          builder.SetProperty(name.to_string(), value.to_string());
        }
      }

      static void ParseBinaryPart(Binary::DataInputStream& stream, Builder& builder)
      {
        while (stream.GetRestSize())
        {
          const uint_t first = stream.Read<le_uint16_t>();
          if ((first & 0xff) == BINARY_SIGNATURE[0] && (first >> 8) == BINARY_SIGNATURE[1])
          {
            // skip possible headers inside
            continue;
          }
          const uint_t last = stream.Read<le_uint16_t>();
          Require(first <= last);
          const std::size_t size = last + 1 - first;
          builder.SetBlock(first, stream.ReadData(size));
        }
      }

    private:
      const Binary::Format::Ptr Format;
    };
  }  // namespace SAP

  Decoder::Ptr CreateSAPDecoder()
  {
    return MakePtr<SAP::Decoder>();
  }
}  // namespace Formats::Multitrack
