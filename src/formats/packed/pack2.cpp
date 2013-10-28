/*
Abstract:
  Pack2 support

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//local includes
#include "container.h"
#include "pack_utils.h"
//common includes
#include <byteorder.h>
#include <pointers.h>
//library includes
#include <formats/packed.h>
//std includes
#include <algorithm>
#include <iterator>
//boost includes
#include <boost/make_shared.hpp>
//text includes
#include <formats/text/packed.h>

namespace Pack2
{
  const std::size_t MAX_DECODED_SIZE = 0xc000;

  const std::string DEPACKER_PATTERN(
    "21??"          // ld hl,xxxx end of packed
    "11??"          // ld de,xxxx end of unpacked
    "e5"            // push hl
    "01??"          // ld bc,xxxx first of unpacked
    "b7"            // or a
    "ed42"          // sbc hl,bc
    "e1"            // pop hl
    "d8"            // ret c
    "7e"            // ld a,(hl)
    "fe?"           // cp xx
    "2804"          // jr z,xx
    "eda8"          // ldd
    "18ee"          // jr
    "2b"            // dec hl
    "46"            // ld b,(hl)
    "cb78"          // bit 7,b
    "2810"          // jr z,xxx
    "cbb8"          // res 7,b
    "78"            // ld a,b
    "a7"            // and a
    "280c"          // jr z,xxx
    "fe?"           // cp xx
    "3e?"           // ld a,xx
    "3006"          // jr nc,xx
    "3e?"           // ld a,xx
    "1802"          // jr xx
    "2b"            // dec hl
    "7e"            // ld a,(hl)
    "12"            // ld (de),a
    "1b"            // dec de
    "10fc"          // djnz xx
    "2b"            // dec hl
    "18cf"          // jr xx
  );

#ifdef USE_PRAGMA_PACK
#pragma pack(push,1)
#endif
  PACK_PRE struct RawHeader
  {
    //+0
    char Padding1;
    //+1
    uint16_t EndOfPacked;
    //+3
    char Padding2;
    //+4
    uint16_t EndOfUnpacked;
    //+6
    char Padding3[2];
    //+8
    uint16_t FirstOfUnpacked;
    //+0xa
    char Padding4[7];
    //+0x11
    uint8_t Marker;
    //+0x12
    char Padding5[0x13];
    //+0x25
    uint8_t RleThreshold;
    //+0x26
    char Padding6;
    //+0x27
    uint8_t SecondRleByte;
    //+0x28
    char Padding7[3];
    //+0x2b
    uint8_t FirstRleByte;
    //+0x2c
    char Padding8[0xb];
    //+0x37

    uint8_t PackedData[1];
  } PACK_POST;
#ifdef USE_PRAGMA_PACK
#pragma pack(pop)
#endif

  BOOST_STATIC_ASSERT(sizeof(RawHeader) == 0x38);

  const std::size_t MIN_SIZE = sizeof(RawHeader);

  class Container
  {
  public:
    Container(const void* data, std::size_t size)
      : Data(static_cast<const uint8_t*>(data))
      , Size(size)
    {
    }

    bool FastCheck() const
    {
      if (Size < sizeof(RawHeader))
      {
        return false;
      }
      const RawHeader& header = GetHeader();
      if (fromLE(header.EndOfPacked) < fromLE(header.FirstOfUnpacked))
      {
        return false;
      }
      if (fromLE(header.EndOfUnpacked) < fromLE(header.EndOfPacked))
      {
        return false;
      }
      const std::size_t usedSize = GetUsedSize();
      if (usedSize > Size)
      {
        return false;
      }
      return true;
    }

    std::size_t GetUsedSize() const
    {
      const RawHeader& header = GetHeader();
      const std::size_t selfAddr = fromLE(header.FirstOfUnpacked) - (sizeof(header) - 1);
      return fromLE(header.EndOfPacked) - selfAddr + 1;
    }

    std::size_t GetPackedSize() const
    {
      const RawHeader& header = GetHeader();
      return fromLE(header.EndOfPacked) - fromLE(header.FirstOfUnpacked);
    }

    const RawHeader& GetHeader() const
    {
      assert(Size >= sizeof(RawHeader));
      return *safe_ptr_cast<const RawHeader*>(Data);
    }
  private:
    const uint8_t* const Data;
    const std::size_t Size;
  };

  class ReverseByteStream
  {
  public:
    ReverseByteStream(const uint8_t* data, std::size_t size)
      : Data(data), Pos(Data + size)
    {
    }

    bool Eof() const
    {
      return Pos < Data;
    }

    uint8_t GetByte()
    {
      return Eof() ? 0 : *--Pos;
    }
  private:
    const uint8_t* const Data;
    const uint8_t* Pos;
  };

  class DataDecoder
  {
  public:
    explicit DataDecoder(const Container& container)
      : IsValid(container.FastCheck())
      , Header(container.GetHeader())
      , Stream(Header.PackedData, container.GetPackedSize())
      , Result(new Dump())
      , Decoded(*Result)
    {
      if (IsValid && !Stream.Eof())
      {
        IsValid = DecodeData();
      }
    }

    std::auto_ptr<Dump> GetResult()
    {
      return IsValid
        ? Result
        : std::auto_ptr<Dump>();
    }
  private:
    bool DecodeData()
    {
      //assume that first byte always exists due to header format
      while (!Stream.Eof() && Decoded.size() < MAX_DECODED_SIZE)
      {
        const uint_t data = Stream.GetByte();
        if (data != Header.Marker)
        {
          Decoded.push_back(data);
        }
        else
        {
          const uint_t token = Stream.GetByte();
          if (token & 0x80)
          {
            if (const std::size_t len = token & 0x7f)
            {
              const uint8_t data = len < Header.RleThreshold
                ? Header.FirstRleByte
                : Header.SecondRleByte;
              std::fill_n(std::back_inserter(Decoded), len, data);
            }
            else
            {
              std::fill_n(std::back_inserter(Decoded), 256, 0);
            }
          }
          else
          {
            const std::size_t len = token ? token : 256;
            const uint8_t data = Stream.GetByte();
            std::fill_n(std::back_inserter(Decoded), len, data);
          }
        }
      }
      Decoded.pop_back();
      std::reverse(Decoded.begin(), Decoded.end());
      return true;
    }
  private:
    bool IsValid;
    const RawHeader& Header;
    ReverseByteStream Stream;
    std::auto_ptr<Dump> Result;
    Dump& Decoded;
  };
}

namespace Formats
{
  namespace Packed
  {
    class Pack2Decoder : public Decoder
    {
    public:
      Pack2Decoder()
        : Depacker(Binary::Format::Create(Pack2::DEPACKER_PATTERN, Pack2::MIN_SIZE))
      {
      }

      virtual String GetDescription() const
      {
        return Text::PACK2_DECODER_DESCRIPTION;
      }

      virtual Binary::Format::Ptr GetFormat() const
      {
        return Depacker;
      }

      virtual Container::Ptr Decode(const Binary::Container& rawData) const
      {
        if (!Depacker->Match(rawData))
        {
          return Container::Ptr();
        }
        const Pack2::Container container(rawData.Start(), rawData.Size());
        if (!container.FastCheck())
        {
          return Container::Ptr();
        }
        Pack2::DataDecoder decoder(container);
        return CreatePackedContainer(decoder.GetResult(), container.GetUsedSize());
      }
    private:
      const Binary::Format::Ptr Depacker;
    };

    Decoder::Ptr CreatePack2Decoder()
    {
      return boost::make_shared<Pack2Decoder>();
    }
  }
}
