/**
 *
 * @file
 *
 * @brief  USF parser implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include "formats/chiptune/emulation/ultra64soundformat.h"
// common includes
#include <byteorder.h>
#include <make_ptr.h>
// library includes
#include <binary/format_factories.h>
#include <binary/input_stream.h>
// std includes
#include <array>

namespace Formats::Chiptune
{
  namespace Ultra64SoundFormat
  {
    const auto DESCRIPTION = "Ultra64 Sound Format"sv;

    using SignatureType = std::array<uint8_t, 4>;
    const SignatureType EMPTY_SIGNATURE = {{0, 0, 0, 0}};
    const SignatureType SR64_SIGNATURE = {{'S', 'R', '6', '4'}};

    class SectionFormat
    {
    public:
      explicit SectionFormat(const Binary::View& data)
        : Stream(data)
      {}

      bool HasSection()
      {
        const auto& sign = Stream.Read<SignatureType>();
        if (sign == SR64_SIGNATURE)
        {
          return true;
        }
        else if (sign == EMPTY_SIGNATURE)
        {
          return false;
        }
        else
        {
          Require(!"Invalid signature");
          return false;
        }
      }

      void ParseROM(Builder& target)
      {
        while (const std::size_t size = Stream.Read<le_uint32_t>())
        {
          const std::size_t offset = Stream.Read<le_uint32_t>();
          target.SetRom(offset, Stream.ReadData(size));
        }
      }

      void ParseSavestate(Builder& target)
      {
        while (const std::size_t size = Stream.Read<le_uint32_t>())
        {
          const std::size_t offset = Stream.Read<le_uint32_t>();
          target.SetSaveState(offset, Stream.ReadData(size));
        }
        /*
        const auto offset = Stream.GetPosition();
        Require(SAVESTATE_SIGNATURE == Stream.Read<SignatureType>());
        const uint_t rdramSize = Stream.Read<le_uint32_t>();
        const auto romHeader = Stream.ReadData(0x40);
        Stream.Seek(offset);
        target.SetSaveState(*Stream.ReadData(0x175c + rdramSize));
        target.SetRom(0, *romHeader);
        */
      }

    private:
      Binary::DataInputStream Stream;
    };

    void ParseSection(Binary::View data, Builder& target)
    {
      SectionFormat format(data);
      if (format.HasSection())
      {
        format.ParseROM(target);
      }
      if (format.HasSection())
      {
        format.ParseSavestate(target);
      }
    }

    const auto FORMAT =
        "'P'S'F"
        "21"
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

      Formats::Chiptune::Container::Ptr Decode(const Binary::Container& /*rawData*/) const override
      {
        return {};  // TODO
      }

    private:
      const Binary::Format::Ptr Format;
    };
  }  // namespace Ultra64SoundFormat

  Decoder::Ptr CreateUSFDecoder()
  {
    return MakePtr<Ultra64SoundFormat::Decoder>();
  }
}  // namespace Formats::Chiptune
