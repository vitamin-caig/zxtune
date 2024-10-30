/**
 *
 * @file
 *
 * @brief  LaserCompact v4.0 support implementation
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
#include <binary/input_stream.h>
#include <formats/image.h>
// std includes
#include <memory>

namespace Formats::Image
{
  namespace LaserCompact40
  {
    const auto DESCRIPTION = "LaserCompact 4.0"sv;
    const auto DEPACKER_PATTERN =
        "0ef9"    // ld c,#f9
        "0d"      // dec c
        "cdc61f"  // call #1fc6
        "119000"  // ld de,#0090
        "19"      // add hl,de
        "1100?"   // ld de,xxxx
        "7e"      // ld a,(hl)
        "d6c0"    // sub #c0
        "381a"    // jr c,xx
        "23"      // inc hl
        "c8"      // ret z
        "1f"      // rra
        "47"      // ld b,a
        "7e"      // ld a,(hl)
        "3045"    // jr nc,xx
        ""sv;

    /*
      @1fc6 48ROM

      e1 pop hl
      c8 ret z
      e9 jp (hl)
    */

    const std::size_t DEPACKER_SIZE = 0x95;

    const std::size_t MIN_SIZE = DEPACKER_SIZE + 16;

    const std::size_t PIXELS_SIZE = 6144;
    const std::size_t ATTRS_SIZE = 768;

    class Container
    {
    public:
      explicit Container(Binary::View data)
        : Data(data.SubView(DEPACKER_SIZE))
      {}

      bool FastCheck() const
      {
        if (!Data)
        {
          return false;
        }
        const uint_t sizeCode = *Data.As<uint8_t>();
        return sizeCode == 0 || sizeCode == 1 || sizeCode == 2 || sizeCode == 8 || sizeCode == 9 || sizeCode == 16;
      }

      Binary::DataInputStream GetStream() const
      {
        return Binary::DataInputStream(Data);
      }

    private:
      const Binary::View Data;
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
      explicit DataDecoder(const Container& container)
        : IsValid(container.FastCheck())
        , Stream(container.GetStream())
      {
        if (IsValid)
        {
          IsValid = DecodeData();
        }
      }

      std::unique_ptr<Binary::Dump> GetResult()
      {
        return IsValid ? std::move(Result) : std::unique_ptr<Binary::Dump>();
      }

      std::size_t GetUsedSize() const
      {
        return Stream.GetPosition();
      }

    private:
      bool DecodeData()
      {
        try
        {
          Binary::Dump decoded(PIXELS_SIZE + ATTRS_SIZE);
          std::fill_n(&decoded[PIXELS_SIZE], ATTRS_SIZE, 7);

          const AddrTranslator translate(Stream.ReadByte());

          std::size_t target = translate.GetStart();
          for (;;)
          {
            const uint8_t val = Stream.ReadByte();
            if (val == 0xc0)
            {
              break;
            }
            if ((val & 0xc1) == 0xc1)
            {
              uint8_t len = (val & 0x3c) >> 2;
              if (0 == len)
              {
                len = Stream.ReadByte();
              }
              else if ((val & 0x02) == 0x00)
              {
                ++len;
              }
              const uint8_t fill = (val & 0x02) == 0x00 ? Stream.ReadByte() : 0;
              do
              {
                decoded.at(translate(target++)) = fill;
              } while (--len);
            }
            else if ((val & 0xc1) == 0xc0)
            {
              uint8_t len = (val & 0x3e) >> 1;
              do
              {
                decoded.at(translate(target++)) = Stream.ReadByte();
              } while (--len);
            }
            else
            {
              uint8_t len = (val & 0x78) >> 3;
              uint8_t hi = (val & 0x07) | 0xf8;

              if (0 == len)
              {
                const uint8_t next = Stream.ReadByte();
                hi = (hi << 1) | (next & 1);
                len = next >> 1;
              }
              ++len;
              uint16_t source = target + (uint_t(hi << 8) | Stream.ReadByte());
              const int_t step = (val & 0x80) ? -1 : +1;
              do
              {
                decoded.at(translate(target++)) = decoded.at(translate(source));
                source += step;
              } while (--len);
            }
          }
          Result = std::make_unique<Binary::Dump>();
          if (target <= PIXELS_SIZE)
          {
            decoded.resize(PIXELS_SIZE);
          }
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
      Binary::DataInputStream Stream;
      std::unique_ptr<Binary::Dump> Result;
    };
  }  // namespace LaserCompact40

  class LaserCompact40Decoder : public Decoder
  {
  public:
    LaserCompact40Decoder()
      : Depacker(Binary::CreateFormat(LaserCompact40::DEPACKER_PATTERN, LaserCompact40::MIN_SIZE))
    {}

    StringView GetDescription() const override
    {
      return LaserCompact40::DESCRIPTION;
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
      const LaserCompact40::Container container(rawData);
      if (!container.FastCheck())
      {
        return {};
      }
      LaserCompact40::DataDecoder decoder(container);
      return CreateContainer(decoder.GetResult(), decoder.GetUsedSize());
    }

  private:
    const Binary::Format::Ptr Depacker;
  };

  Decoder::Ptr CreateLaserCompact40Decoder()
  {
    return MakePtr<LaserCompact40Decoder>();
  }
}  // namespace Formats::Image
