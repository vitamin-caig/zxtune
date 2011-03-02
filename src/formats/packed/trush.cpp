/*
Abstract:
  Trush support

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
  (C) Based on XLook sources by HalfElf
*/

//local includes
#include "hrust1_bitstream.h"
#include "pack_utils.h"
//common includes
#include <byteorder.h>
#include <detector.h>
#include <tools.h>
//library includes
#include <formats/packed.h>
//std includes
#include <cstring>

namespace TRUSH
{
  const std::size_t MAX_DECODED_SIZE = 0xc000;

  const char TRUSH_SIGNATURE[] =
  {
    'C', 'O', 'M', 'P', 'R', 'E', 'S', 'S', 'O', 'R', ' ', 'B', 'Y', ' ',
    'A', 'L', 'E', 'X', 'A', 'N', 'D', 'E', 'R', ' ', 'T', 'R', 'U', 'S', 'H', ' ', 'O', 'D', 'E', 'S', 'S', 'A'
  };

#ifdef USE_PRAGMA_PACK
#pragma pack(push,1)
#endif
  PACK_PRE struct RawHeader
  {
    //+0
    char Padding1[0x0c];
    //+c
    uint16_t SizeOfPacked;
    //+e
    char Padding2[0x19];
    //+27
    char Signature[0x24];
    //+4b
    char Padding3[0xce];
    //+119
    uint8_t BitStream[2];
  } PACK_POST;
#ifdef USE_PRAGMA_PACK
#pragma pack(pop)
#endif

  BOOST_STATIC_ASSERT(sizeof(RawHeader) == 0x11b);

  class Container
  {
  public:
    Container(const void* data, std::size_t size)
      : Data(static_cast<const uint8_t*>(data))
      , Size(size)
    {
    }

    bool Check() const
    {
      if (Size < sizeof(RawHeader))
      {
        return false;
      }
      const RawHeader& header = GetHeader();
      BOOST_STATIC_ASSERT(sizeof(header.Signature) == sizeof(TRUSH_SIGNATURE));
      if (0 != std::memcmp(header.Signature, TRUSH_SIGNATURE, sizeof(header.Signature)))
      {
        return false;
      }
      const std::size_t usedSize = GetUsedSize();
      return in_range(usedSize, sizeof(header), Size);
    }

    uint_t GetUsedSize() const
    {
      const RawHeader& header = GetHeader();
      return sizeof(header) + fromLE(header.SizeOfPacked) - sizeof(header.BitStream);
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

  class DataDecoder
  {
  public:
    explicit DataDecoder(const Container& container)
      : IsValid(container.Check())
      , Header(container.GetHeader())
      , Stream(Header.BitStream, fromLE(Header.SizeOfPacked))
    {
      assert(!Stream.Eof());
    }

    Dump* GetDecodedData()
    {
      if (IsValid && !Stream.Eof())
      {
        IsValid = DecodeData();
      }
      return IsValid ? &Decoded : 0;
    }
  private:
    bool DecodeData()
    {
      const uint_t packedSize = fromLE(Header.SizeOfPacked);

      Decoded.reserve(packedSize * 2);

      while (!Stream.Eof() && Decoded.size() < MAX_DECODED_SIZE)
      {
        //%0 - put byte
        if (!Stream.GetBit())
        {
          Decoded.push_back(Stream.GetByte());
          continue;
        }
        uint_t code = Stream.GetBits(2);
        if (code)
        {
          code = 2 * code + Stream.GetBit();
          if (code >= 6)
          {
            code = 2 * code + Stream.GetBit();
          }
        }
        if (2 == code)
        {
          const uint_t offset = Stream.GetByte() + 1;
          if (!CopyFromBack(offset, Decoded, 2))
          {
            return false;
          }
          continue;
        }
        uint_t len = code < 2 ? code + 3 : 0;
        if (3 == code)
        {
          const uint_t data = Stream.GetByte();
          if (0xff == data)
          {
            break;
          }
          else if (0xfe == data)
          {
            len = Stream.GetLEWord();
          }
          else
          {
            len = data + 10;
          }
        }
        else if (code > 3)
        {
          len = code;
          if (len >= 6)
          {
            len -= 6;
          }
        }
        const uint_t offset = GetOffset();
        if (!CopyFromBack(offset, Decoded, len))
        {
          return false;
        }
      }
      return true;
    }

    uint_t GetOffset()
    {
      if (Stream.GetBit())
      {
        return Stream.GetByte() + 1;
      }
      const uint_t code1 = Stream.GetBits(3);
      if (code1 < 2)
      {
        return 256 * (code1 + 1) + Stream.GetByte() + 1;
      }
      const uint_t code2 = 2 * code1 + Stream.GetBit();
      if (code2 < 8)
      {
        return 256 * (code2 - 1) + Stream.GetByte() + 1;
      }
      const uint_t code3 = 2 * code2 + Stream.GetBit();
      if (code3 < 0x17)
      {
        return 256 * (code3 - 9) + Stream.GetByte() + 1;
      }
      const uint_t code4 = (2 * code3 + Stream.GetBit()) & 0x1f;
      const uint_t lodisp = Stream.GetByte();
      return 256 * code4 + lodisp + 1;
    }
  private:
    bool IsValid;
    const RawHeader& Header;
    Hrust1Bitstream Stream;
    Dump Decoded;
  };
}


namespace Formats
{
  namespace Packed
  {
    class TRUSHDecoder : public Decoder
    {
    public:
      virtual bool Check(const void* data, std::size_t availSize) const
      {
        const TRUSH::Container container(data, availSize);
        return container.Check();
      }

      virtual std::auto_ptr<Dump> Decode(const void* data, std::size_t availSize, std::size_t& usedSize) const
      {
        const TRUSH::Container container(data, availSize);
        assert(container.Check());
        TRUSH::DataDecoder decoder(container);
        if (Dump* decoded = decoder.GetDecodedData())
        {
          usedSize = container.GetUsedSize();
          std::auto_ptr<Dump> res(new Dump());
          res->swap(*decoded);
          return res;
        }
        return std::auto_ptr<Dump>();
      }
    };

    Decoder::Ptr CreateTRUSHDecoder()
    {
      return Decoder::Ptr(new TRUSHDecoder());
    }
  }
}
