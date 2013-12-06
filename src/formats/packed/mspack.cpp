/**
* 
* @file
*
* @brief  MicrospacePacker packer support
*
* @author vitamin.caig@gmail.com
*
* @note   Based on XLook sources by HalfElf
*
**/

//local includes
#include "container.h"
#include "hrust1_bitstream.h"
#include "pack_utils.h"
//common includes
#include <byteorder.h>
#include <pointers.h>
//library includes
#include <binary/format_factories.h>
#include <formats/packed.h>
#include <math/numeric.h>
//std includes
#include <algorithm>
#include <iterator>
#include <cstring>
//boost includes
#include <boost/make_shared.hpp>
//text includes
#include <formats/text/packed.h>

namespace MSPack
{
  const std::size_t MAX_DECODED_SIZE = 0xc000;

  const std::string DEPACKER_PATTERN(
    "'M's'P'k"
  );

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
    uint16_t SrcPacked;
    //+8
    uint16_t DstPacked;
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

  const std::size_t MIN_SIZE = sizeof(RawHeader);

  const std::size_t LAST_BYTES_COUNT = 5;

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
      if (Size <= sizeof(RawHeader))
      {
        return false;
      }
      const RawHeader& header = GetHeader();
      const uint_t endOfBlock = fromLE(header.LastSrcRestBytes) + 1;
      const uint_t lastBytesAddr = endOfBlock - LAST_BYTES_COUNT;
      const uint_t bitStreamAddr = lastBytesAddr - fromLE(header.SizeOfPacked);

      const uint_t srcPacked = fromLE(header.SrcPacked);
      if (bitStreamAddr == srcPacked)
      {
        //move forward
      }
      else if (lastBytesAddr == srcPacked + 1)
      {
        //move backward
      }
      else
      {
        return false;
      }
      const std::size_t usedSize = GetUsedSize();
      return Math::InRange(usedSize, sizeof(header), Size);
    }

    uint_t GetUsedSize() const
    {
      const RawHeader& header = GetHeader();
      return offsetof(RawHeader, DstAddress) + fromLE(header.SizeOfPacked) + LAST_BYTES_COUNT;
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
      : IsValid(container.FastCheck())
      , Header(container.GetHeader())
      , Stream(Header.BitStream, fromLE(Header.SizeOfPacked))
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
      Decoded.reserve(MAX_DECODED_SIZE);

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
          const uint_t disp = Stream.GetByte() + 1;
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
      const uint8_t* const lastBytes = Header.BitStream + Stream.GetProcessedBytes();
      std::copy(lastBytes, lastBytes + LAST_BYTES_COUNT, std::back_inserter(Decoded));
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
    std::auto_ptr<Dump> Result;
    Dump& Decoded;
  };
}

namespace Formats
{
  namespace Packed
  {
    class MSPackDecoder : public Decoder
    {
    public:
      MSPackDecoder()
        : Depacker(Binary::CreateFormat(MSPack::DEPACKER_PATTERN, MSPack::MIN_SIZE))
      {
      }

      virtual String GetDescription() const
      {
        return Text::MSP_DECODER_DESCRIPTION;
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
        const MSPack::Container container(rawData.Start(), rawData.Size());
        if (!container.FastCheck())
        {
          return Container::Ptr();
        }
        MSPack::DataDecoder decoder(container);
        return CreatePackedContainer(decoder.GetResult(), container.GetUsedSize());
      }
    private:
    const Binary::Format::Ptr Depacker;
    };

    Decoder::Ptr CreateMSPackDecoder()
    {
      return boost::make_shared<MSPackDecoder>();
    }
  }
}
