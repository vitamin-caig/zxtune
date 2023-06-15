/**
 *
 * @file
 *
 * @brief  ASCScreenCrusher support implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include "formats/image/container.h"
// common includes
#include <contract.h>
#include <make_ptr.h>
// library includes
#include <binary/format_factories.h>
#include <formats/image.h>
// std includes
#include <memory>

namespace Formats::Image
{
  namespace ASCScreenCrusher
  {
    const Char DESCRIPTION[] = "ASC ScreenCrasher";
    const auto DEPACKER_PATTERN =
        "f3"      // di
        "cd5200"  // call #0052
        "3b"      // dec sp
        "3b"      // dec sp
        "c1"      // pop bc
        "219700"  // ld hl,#0097
        "09"      // add hl,bc
        "eb"      // ex de,hl
        "("
        "21?00"  // ld hl,#00xx
        "09"     // add hl,bc
        "73"     // ld (hl),e
        "23"     // inc hl
        "72"     // ld (hl),d
        "){3}"
        "21be00"  // ld hl,#00be
        "09"      // add hl,bc
        "1100?"   // ld de,#4000
        "d5"      // push de
        ""_sv;

    /*
      @0052 48ROM

      c9 ret
    */

    const std::size_t DEPACKER_SIZE = 0xc2;
    const std::size_t MIN_SIZE = DEPACKER_SIZE + 16;

    const std::size_t PIXELS_SIZE = 6144;
    const std::size_t ATTRS_SIZE = 768;

    class ByteStream
    {
    public:
      ByteStream(const uint8_t* data, std::size_t size, std::size_t offset)
        : Start(data)
        , Cursor(Start + offset)
        , End(Start + size)
      {}

      uint8_t GetByte()
      {
        Require(Cursor != End);
        return *Cursor++;
      }

      std::size_t GetProcessedBytes() const
      {
        return Cursor - Start;
      }

    private:
      const uint8_t* const Start;
      const uint8_t* Cursor;
      const uint8_t* const End;
    };

    class AddrTranslator
    {
    public:
      AddrTranslator(uint_t sizeCode)
        : AttrBase(256 * sizeCode)
        , ScrStart((AttrBase & 0x0300) << 3)
        , ScrLimit((AttrBase ^ 0x1800) & 0xfc00)
      {}

      std::size_t GetStart() const
      {
        return ScrStart;
      }

      std::size_t operator()(std::size_t virtAddr) const
      {
        if (virtAddr < ScrLimit)
        {
          const std::size_t line = (virtAddr & 0x0007) << 8;
          const std::size_t row = (virtAddr & 0x0038) << 2;
          const std::size_t col = (virtAddr & 0x07c0) >> 6;
          return (virtAddr & 0x1800) | line | row | col;
        }
        else
        {
          return AttrBase + virtAddr;
        }
      }

    private:
      const std::size_t AttrBase;
      const std::size_t ScrStart;
      const std::size_t ScrLimit;
    };

    class DataDecoder
    {
    public:
      explicit DataDecoder(Binary::View data)
        : Stream(data.As<uint8_t>(), data.Size(), DEPACKER_SIZE)
      {
        IsValid = DecodeData();
      }

      std::unique_ptr<Binary::Dump> GetResult()
      {
        return IsValid ? std::move(Result) : std::unique_ptr<Binary::Dump>();
      }

      std::size_t GetUsedSize() const
      {
        return Stream.GetProcessedBytes();
      }

    private:
      bool DecodeData()
      {
        try
        {
          Binary::Dump decoded(PIXELS_SIZE + ATTRS_SIZE);
          std::fill_n(&decoded[PIXELS_SIZE], ATTRS_SIZE, 7);

          const AddrTranslator translate(0);

          std::size_t target = 0;
          for (;;)
          {
            const uint8_t val = Stream.GetByte();
            if (val == 0x80)
            {
              break;
            }
            switch (val & 0xc0)
            {
            case 0x80:
              for (uint8_t len = val & 0x3f; len != 0; --len)
              {
                decoded.at(translate(target++)) = Stream.GetByte();
              }
              break;
            case 0xc0:
              for (uint8_t len = (val & 0x3f) + 3, fill = Stream.GetByte(); len != 0; --len)
              {
                decoded.at(translate(target++)) = fill;
              }
              break;
            default:
            {
              uint16_t source = target - (256 * (val & 0x07) + Stream.GetByte());
              for (uint8_t len = ((val & 0xf8) >> 3) + 3; len != 0; --len)
              {
                decoded.at(translate(target++)) = decoded.at(translate(source++));
              }
            }
            }
          }
          Require(target == decoded.size());
          Result = std::make_unique<Binary::Dump>();
          Result->swap(decoded);
          return true;
        }
        catch (const std::exception&)
        {
          return false;
        }
      }

    private:
      bool IsValid;
      ByteStream Stream;
      std::unique_ptr<Binary::Dump> Result;
    };
  }  // namespace ASCScreenCrusher

  class ASCScreenCrusherDecoder : public Decoder
  {
  public:
    ASCScreenCrusherDecoder()
      : Depacker(Binary::CreateFormat(ASCScreenCrusher::DEPACKER_PATTERN, ASCScreenCrusher::MIN_SIZE))
    {}

    String GetDescription() const override
    {
      return ASCScreenCrusher::DESCRIPTION;
    }

    Binary::Format::Ptr GetFormat() const override
    {
      return Depacker;
    }

    Container::Ptr Decode(const Binary::Container& rawData) const override
    {
      if (!Depacker->Match(rawData))
      {
        return {};
      }
      ASCScreenCrusher::DataDecoder decoder(rawData);
      return CreateContainer(decoder.GetResult(), decoder.GetUsedSize());
    }

  private:
    const Binary::Format::Ptr Depacker;
  };

  Decoder::Ptr CreateASCScreenCrusherDecoder()
  {
    return MakePtr<ASCScreenCrusherDecoder>();
  }
}  // namespace Formats::Image
