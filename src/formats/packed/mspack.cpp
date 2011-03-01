/*
Abstract:
  MS Packer support

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
#include <algorithm>
#include <iterator>
#include <cstring>

namespace MSPack
{
  const std::size_t MAX_DECODED_SIZE = 0xc000;

  const char MSP_SIGNATURE[] = {'M', 's', 'P', 'k'};

#ifdef USE_PRAGMA_PACK
#pragma pack(push,1)
#endif
  PACK_PRE struct RawHeader
  {
    //+0
    char Signature[4]; //'M','s','P','k'
    //+4
    uint16_t LastSrcRestBytes;
    //+6
    uint16_t LastSrcPacked;
    //+8
    uint16_t LastDstPacked;
    //+a
    uint16_t SizeOfPacked;
    //+c
    uint16_t DstAddress;
    //+e
    uint8_t BitStream[2];
  } PACK_POST;
#ifdef USE_PRAGMA_PACK
#pragma pack(pop)
#endif

  BOOST_STATIC_ASSERT(sizeof(RawHeader) == 0x10);

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
      if (sizeof(RawHeader) > Size)
      {
        return false;
      }
      const RawHeader& header = GetHeader();
      BOOST_STATIC_ASSERT(sizeof(header.Signature) == sizeof(MSP_SIGNATURE));
      if (0 != std::memcmp(header.Signature, MSP_SIGNATURE, sizeof(header.Signature)))
      {
        return false;
      }
      if (fromLE(header.LastSrcRestBytes) <= fromLE(header.LastSrcPacked) ||
          fromLE(header.LastDstPacked) <= fromLE(header.DstAddress))
      {
        return false;
      }
      if (fromLE(header.LastSrcRestBytes) <= fromLE(header.LastSrcPacked))
      {
        return false;
      }
      const std::size_t usedSize = GetUsedSize();
      return in_range(usedSize, sizeof(header), Size);
    }

    uint_t GetUsedSize() const
    {
      const RawHeader& header = GetHeader();
      const uint_t lastBytesCount = fromLE(header.LastSrcRestBytes) - fromLE(header.LastSrcPacked);
      return sizeof(header) + fromLE(header.SizeOfPacked) + lastBytesCount - 4;
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
      const unsigned lastBytesCount = fromLE(Header.LastSrcRestBytes) - fromLE(Header.LastSrcPacked);
      const uint_t packedSize = fromLE(Header.SizeOfPacked);
      const unsigned unpackedSize = fromLE(Header.LastDstPacked) - fromLE(Header.DstAddress) + 1;

      Decoded.reserve(unpackedSize);

      while (!Stream.Eof() && Decoded.size() < MAX_DECODED_SIZE)
      {
        //%0 - put byte
        if (!Stream.GetBit())
        {
          Decoded.push_back(Stream.GetByte());
          continue;
        }
        uint_t len = Stream.GetLen();
        len += 2;
        if (2 == len)
        {
          const unsigned disp = Stream.GetByte() + 1;
          if (!CopyFromBack(disp, Decoded, 2))
          {
            return false;
          }
          continue;
        }
        if (5 == len)
        {
          const uint_t data = Stream.GetByte();
          if (0xff == data)
          {
            break;
          }
          if (0xfe == data)
          {
            len = Stream.GetLEWord();
          }
          else
          {
            len = data + 17;
          }
        }
        else if (len > 5)
        {
          --len;
        }
        const uint_t disp = GetOffset();
        if (!CopyFromBack(disp, Decoded, len))
        {
          return false;
        }
      }
      const uint8_t* const lastBytes = Header.BitStream + packedSize - sizeof(Header.BitStream);
      std::copy(lastBytes, lastBytes + lastBytesCount, std::back_inserter(Decoded));
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
      if (code4 < 0x1f)
      {
        return 256 * code4 + Stream.GetByte() + 1;
      }
      const uint_t hidisp = Stream.GetByte();
      const uint_t lodisp = Stream.GetByte();
      return 256 * hidisp + lodisp + 1;
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
    class MSPackDecoder : public Decoder
    {
    public:
      virtual bool Check(const void* data, std::size_t availSize) const
      {
        const MSPack::Container container(data, availSize);
        return container.Check();
      }

      virtual std::auto_ptr<Dump> Decode(const void* data, std::size_t availSize, std::size_t& usedSize) const
      {
        const MSPack::Container container(data, availSize);
        assert(container.Check());
        MSPack::DataDecoder decoder(container);
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

    Decoder::Ptr CreateMSPackDecoder()
    {
      return Decoder::Ptr(new MSPackDecoder());
    }
  }
}
