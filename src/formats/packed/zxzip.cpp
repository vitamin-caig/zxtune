/*
Abstract:
  ZXZip support

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
  (C) Based on XLook sources by HalfElf
*/

//local includes
#include "pack_utils.h"
//common includes
#include <byteorder.h>
#include <crc.h>
#include <detector.h>
#include <tools.h>
//library includes
#include <formats/packed.h>
//std includes
#include <cstring>
//text includes
#include <core/text/plugins.h>

namespace ZXZip
{
  const std::size_t MAX_DECODED_SIZE = 0xff00;
  //checkers
  const std::string HEADER_PATTERN =
    //Filename
    "20-7a 20-7a 20-7a 20-7a 20-7a 20-7a 20-7a 20-7a"
    //Type
    "20-7a ??"
    //SourceSize
    "??"
    //SourceSectors
    "01-ff"
    //PackedSize
    "??"
    //SourceCRC
    "????"
    //Method
    "00-03"
    //Flags
    "%0000000x"
  ;

#ifdef USE_PRAGMA_PACK
#pragma pack(push,1)
#endif
  PACK_PRE struct RawHeader
  {
    //+0x0
    char Name[8];
    //+0x8
    char Type;
    //+0x9
    uint16_t StartOrSize;
    //+0xb
    uint16_t SourceSize;
    //+0xd
    uint8_t SourceSectors;
    //+0xe
    uint16_t PackedSize;
    //+0x10
    uint32_t SourceCRC;
    //+0x14
    uint8_t Method;
    //+0x15
    uint8_t Flags;
    //+0x16
  } PACK_POST;
#ifdef USE_PRAGMA_PACK
#pragma pack(pop)
#endif

  BOOST_STATIC_ASSERT(sizeof(RawHeader) == 0x16);

  uint_t GetSourceFileSize(const RawHeader& header)
  {
    const uint_t calcSize = header.Type == 'B' || header.Type == 'b'
      ? 4 + fromLE(header.StartOrSize)
      : fromLE(header.SourceSize);
    const uint_t calcSectors = align(calcSize, uint_t(256)) / 256;
    return calcSectors == header.SourceSectors
      ? calcSize
      : 256 * header.SourceSectors;
  }

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
      const uint_t usedSize = GetUsedSize();
      return usedSize - sizeof(header) <= fromLE(header.PackedSize) &&
             usedSize <= Size;
    }

    uint_t GetUsedSize() const
    {
      const RawHeader& header = GetHeader();
      return sizeof(header) + fromLE(header.PackedSize);
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
    {
      assert(IsValid);
    }

    Dump* GetDecodedData()
    {
      if (!IsValid)
      {
        return 0;
      }
      switch (Header.Method)
      {
      case 0:
        IsValid = DeStoreData();
        break;
      default:
        IsValid = false;
        break;
      }
      return IsValid ? &Decoded : 0;
    }
  private:
    bool DeStoreData()
    {
      const uint_t dataSize = GetSourceFileSize(Header);
      Decoded.resize(dataSize);
      std::memcpy(&Decoded[0], &Header + 1, dataSize);
      return CheckCRC();
    }

    bool CheckCRC() const
    {
      const uint32_t realCRC = Crc32(&Decoded[0], Decoded.size());
      //ZXZip CRC32 calculation does not invert result
      return realCRC == ~fromLE(Header.SourceCRC);
    }
  private:
    bool IsValid;
    const RawHeader& Header;
    Dump Decoded;
  };
}

namespace Formats
{
  namespace Packed
  {
    class ZXZipDecoder : public Decoder
    {
    public:
      ZXZipDecoder()
        : Depacker(DataFormat::Create(ZXZip::HEADER_PATTERN))
      {
      }

      virtual DataFormat::Ptr GetFormat() const
      {
        return Depacker;
      }

      virtual bool Check(const void* data, std::size_t availSize) const
      {
        const ZXZip::Container container(data, availSize);
        return container.FastCheck() && Depacker->Match(data, availSize);
      }

      virtual std::auto_ptr<Dump> Decode(const void* data, std::size_t availSize, std::size_t& usedSize) const
      {
        const ZXZip::Container container(data, availSize);
        if (!container.FastCheck() || !Depacker->Match(data, availSize))
        {
          return std::auto_ptr<Dump>();
        }
        ZXZip::DataDecoder decoder(container);
        if (Dump* decoded = decoder.GetDecodedData())
        {
          usedSize = container.GetUsedSize();
          std::auto_ptr<Dump> res(new Dump());
          res->swap(*decoded);
          return res;
        }
        return std::auto_ptr<Dump>();
      }
    private:
      const DataFormat::Ptr Depacker;
    };

    Decoder::Ptr CreateZXZipDecoder()
    {
      return Decoder::Ptr(new ZXZipDecoder());
    }
  }
}
