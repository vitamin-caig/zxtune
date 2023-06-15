/**
 *
 * @file
 *
 * @brief  MegaLZ packer support
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include "formats/packed/container.h"
#include "formats/packed/pack_utils.h"
// common includes
#include <byteorder.h>
#include <contract.h>
#include <make_ptr.h>
// library includes
#include <binary/format_factories.h>
#include <formats/packed.h>
// std includes
#include <numeric>

namespace Formats::Packed
{
  namespace MegaLZ
  {
    const std::size_t MIN_DECODED_SIZE = 0x100;
    const std::size_t DEPACKER_SIZE = 110;
    const std::size_t MIN_SIZE = 256;
    const std::size_t MAX_DECODED_SIZE = 0xc000;

    const Char DESCRIPTION[] = "MegaLZ";
    // assume that packed data are located right after depacked
    // prologue is ignored due to standard absense
    const auto DEPACKER_PATTERN =
        "3e80"    // ld a,#80
        "08"      // ex af,af'
        "eda0"    // ldi
        "01ff02"  // ld bc,#02ff
        "08"      // ex af,af'
        "87"      // add a,a
        "2003"    // jr nz,xxx
        "7e"      // ld a,(hl)
        "23"      // inc hl
        "17"      // rla
        "cb11"    // rl c
        "30f6"    // jr nc,xxx
        "08"      // ex af,af'
        "100f"    // djnz xxxx
        "3e02"    // ld a,2
        "cb29"    // sra c
        "3818"    // jr c,xxxx
        "3c"      // inc a
        "0c"      // inc c
        "280f"    // jr z,xxxx
        "013f03"  // ld bc,#033f
        ""_sv;

    class Bitstream : private ByteStream
    {
    public:
      explicit Bitstream(Binary::View data)
        : ByteStream(data.As<uint8_t>(), data.Size())
      {}

      uint8_t GetByte()
      {
        Require(!Eof());
        return ByteStream::GetByte();
      }

      uint_t GetBit()
      {
        if (!(Mask >>= 1))
        {
          Bits = GetByte();
          Mask = 0x80;
        }
        return Bits & Mask ? 1 : 0;
      }

      uint_t GetBits(uint_t count)
      {
        uint_t result = 0;
        while (count--)
        {
          result = 2 * result | GetBit();
        }
        return result;
      }

      uint_t GetLen()
      {
        uint_t len = 1;
        while (!GetBit())
        {
          ++len;
        }
        return len;
      }

      int_t GetDist()
      {
        if (GetBit())
        {
          const uint_t bits = GetBits(4) << 8;
          return static_cast<int16_t>(((0xf000 | bits) - 0x100) | GetByte());
        }
        else
        {
          return static_cast<int16_t>(0xff00 + GetByte());
        }
      }

      using ByteStream::GetProcessedBytes;

    private:
      uint_t Bits = 0;
      uint_t Mask = 0;
    };

    class DataDecoder
    {
    public:
      explicit DataDecoder(Binary::View data)
        : IsValid(data.Size() >= MIN_SIZE)
        , Stream(data.SubView(DEPACKER_SIZE))
      {
        if (IsValid)
        {
          Decoded = Binary::DataBuilder(data.Size() * 2);
          IsValid = DecodeData();
        }
      }

      Binary::Container::Ptr GetResult()
      {
        return IsValid ? Decoded.CaptureResult() : Binary::Container::Ptr();
      }

      std::size_t GetUsedSize() const
      {
        return DEPACKER_SIZE + Stream.GetProcessedBytes();
      }

    private:
      bool DecodeData()
      {
        try
        {
          Decoded.AddByte(Stream.GetByte());
          while (Decoded.Size() < MAX_DECODED_SIZE)
          {
            if (Stream.GetBit())
            {
              Decoded.AddByte(Stream.GetByte());
              continue;
            }
            const uint_t code = Stream.GetBits(2);
            if (code == 0)
            {
              //%0 00
              Require(CopyFromBack(-static_cast<int16_t>(0xfff8 | Stream.GetBits(3)), Decoded, 1));
            }
            else if (code == 1)
            {
              //%0 01
              Require(CopyFromBack(-static_cast<int16_t>(0xff00 | Stream.GetByte()), Decoded, 2));
            }
            else if (code == 2)
            {
              //%0 10
              Require(CopyFromBack(-Stream.GetDist(), Decoded, 3));
            }
            else
            {
              const uint_t len = Stream.GetLen();
              if (len == 9)
              {
                Require(Decoded.Size() >= MIN_DECODED_SIZE);
                return true;
              }
              else
              {
                Require(len <= 7);
                const uint_t bits = Stream.GetBits(len);
                Require(CopyFromBack(-Stream.GetDist(), Decoded, 2 + (1 << len) + bits));
              }
            }
          }
        }
        catch (const std::exception&)
        {}
        return false;
      }

    private:
      bool IsValid;
      Bitstream Stream;
      Binary::DataBuilder Decoded;
    };
  }  // namespace MegaLZ

  class MegaLZDecoder : public Decoder
  {
  public:
    MegaLZDecoder()
      : Depacker(Binary::CreateFormat(MegaLZ::DEPACKER_PATTERN, MegaLZ::MIN_SIZE))
    {}

    String GetDescription() const override
    {
      return MegaLZ::DESCRIPTION;
    }

    Binary::Format::Ptr GetFormat() const override
    {
      return Depacker;
    }

    Container::Ptr Decode(const Binary::Container& rawData) const override
    {
      const Binary::View data(rawData);
      if (!Depacker->Match(data))
      {
        return {};
      }
      MegaLZ::DataDecoder decoder(data.SubView(0, MegaLZ::MAX_DECODED_SIZE));
      return CreateContainer(decoder.GetResult(), decoder.GetUsedSize());
    }

  private:
    const Binary::Format::Ptr Depacker;
  };

  Decoder::Ptr CreateMegaLZDecoder()
  {
    return MakePtr<MegaLZDecoder>();
  }
}  // namespace Formats::Packed
