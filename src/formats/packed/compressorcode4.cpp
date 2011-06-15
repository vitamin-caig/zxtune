	/*
Abstract:
  CC4 support

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
#include <detector.h>
#include <tools.h>
//library includes
#include <formats/packed.h>
//std includes
#include <algorithm>
#include <iterator>
#include <stdexcept>

namespace CompressorCode4
{
  const std::size_t MAX_DECODED_SIZE = 0xc000;

  const std::string DEPACKER_PATTERN(
    "cd5200"  // call 0x52
    "3b"      // dec sp
    "3b"      // dec sp
    "e1"      // pop hl
    "011100"  // ld bc,0x0011 ;0x07
    "09"      // add hl,bc
    "11??"    // ld de,xxxx ;depacker addr
    "01??"    // ld bc,xxxx ;depacker size +e
    "d5"      // push de
    "edb0"    // ldir
    "c9"      // ret
    //add hl,bc points here +14
    "fde5"    // push iy
    "11??"    // ld de,xxxx ;dst addr
    "01??"    // ld bc,xxxx ;packed size +1a
    "c5"      // push bc
    "01??"    // ld bc,xxxx
    "c5"      // push bc
  );

#ifdef USE_PRAGMA_PACK
#pragma pack(push,1)
#endif
  PACK_PRE struct RawHeader
  {
    //+0
    uint8_t Padding1[0x0e];
    //+0x0e
    uint16_t RestDepackerSize;
    //+0x10
    uint8_t Padding2[0x0a];
    //+0x1a
    uint16_t ChunksCount;
    //+0x1c
  } PACK_POST;
#ifdef USE_PRAGMA_PACK
#pragma pack(pop)
#endif

  BOOST_STATIC_ASSERT(sizeof(RawHeader) == 0x1c);

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
      const uint_t depackerSize = 0x14 + fromLE(header.RestDepackerSize);
      const uint_t chunksCount = fromLE(header.ChunksCount);
      //at least one byte per chunk
      if (chunksCount > MAX_DECODED_SIZE ||
          depackerSize + chunksCount > Size)
      {
        return false;
      }
      return Data[depackerSize - 1] == 0xc9;
    }

    uint_t GetAvailableData() const
    {
      return Size;
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

  class StreamAdapter
  {
  public:
    StreamAdapter(const uint8_t* data, std::size_t size)
      : Delegate(data, size)
      , UsedData(0)
    {
    }

    uint_t GetUsedData() const
    {
      return UsedData;
    }

    uint8_t operator * ()
    {
      if (Delegate.Eof())
      {
        throw std::exception();
      }
      ++UsedData;
      return Delegate.GetByte();
    }

    StreamAdapter& operator ++ (int)
    {
      return *this;
    }

    bool Eof() const
    {
      return Delegate.Eof();
    }
  private:
    ByteStream Delegate;
    uint_t UsedData;
  };

  class DataDecoder
  {
  public:
    explicit DataDecoder(const Container& container)
      : IsValid(container.FastCheck())
      , Header(container.GetHeader())
      , DataOffset(0x14 + fromLE(Header.RestDepackerSize))
      , Stream(Header.Padding1 + DataOffset, container.GetAvailableData() - DataOffset)
    {
      assert(IsValid && !Stream.Eof());
    }

    Dump* GetDecodedData()
    {
      if (IsValid && !Stream.Eof())
      {
        IsValid = DecodeData();
      }
      return IsValid ? &Decoded : 0;
    }

    uint_t GetUsedSize() const
    {
      return Stream.GetUsedData() + DataOffset;
    }
  private:
    bool DecodeData()
    {
      try
      {
        uint_t chunksCount = fromLE(Header.ChunksCount);
        Decoded.reserve(2 * chunksCount);
        StreamAdapter& source(Stream);
        std::back_insert_iterator<Dump> target(Decoded);
        //assume that first byte always exists due to header format
        while (chunksCount-- && Decoded.size() < MAX_DECODED_SIZE)
        {
          const uint_t data = *source;
          const uint_t count = (data >> 5);
          if (24 == (data & 24))
          {
            const uint_t offset = 256 * (data & 7) + *source;
            if (!CopyFromBack(offset, Decoded, count + 3))
            {
              return false;
            }
            continue;
          }
          uint_t len = (data & 1)
            ? 256 * count + *source
            : count + 3;
          switch (data & 0x1f)
          {
          case 0:
            len -= 2;
          case 1:
            while (len--)
            {
              *target = *source;
            }
            break;
          case 2:
            --len;
          case 3:
            std::fill_n(target, len, 0);
            break;
          case 4:
            --len;
          case 5:
            std::fill_n(target, len, 0xff);
            break;
          case 6:
          case 7:
            std::fill_n(target, len, *source);
            break;
          case 8:
            ++len;
          case 9:
            for (uint_t data = *source, delta = *source; len; --len, data += delta)
            {
              *target = data;
            }
            break;
          case 0xa:
            --len;
          case 0xb:
            for (const uint_t data1 = *source, data2 = *source; len; --len)
            {
              *target = data1;
              *target = data2;
            }
            break;
          case 0xc:
            --len;
          case 0xd:
            for (const uint_t data1 = *source, data2 = *source, data3 = *source; len; --len)
            {
              *target = data1;
              *target = data2;
              *target = data3;
            }
            break;
          case 0xe:
          case 0xf:
            for (const uint_t data = *source; len; --len)
            {
                *target = data;
                *target = *source;
            }
            break;
          case 0x10:
          case 0x11:
            for (const uint_t data = *source; len; --len)
            {
                *target = data;
                *target = *source;
                *target = *source;
            }
            break;
          case 0x12:
          case 0x13:
            for (const uint_t base = *source; len; --len)
            {
              const uint_t data = *source;
              *target = base + (data >> 4);
              *target = base + (data & 0x0f);
            }
            break;
          case 0x14:
          case 0x15:
            {
              const uint_t offset = *source;
              if (!CopyFromBack(offset, Decoded, len))
              {
                return false;
              }
            }
            break;
          case 0x16:
            ++len;
          case 0x17:
            {
              const uint_t hiOff = *source;
              const uint_t loOff = *source;
              const uint_t offset = 256 * hiOff + loOff;
              if (!CopyFromBack(offset, Decoded, len))
              {
                return false;
              }
            }
            break;
          }
        }
        return true;
      }
      catch (const std::exception&)
      {
        return false;
      }
    }
  private:
    bool IsValid;
    const RawHeader& Header;
    const uint_t DataOffset;
    StreamAdapter Stream;
    Dump Decoded;
  };
}

namespace Formats
{
  namespace Packed
  {
    class CompressorCode4Decoder : public Decoder
    {
    public:
      CompressorCode4Decoder()
        : Depacker(DataFormat::Create(CompressorCode4::DEPACKER_PATTERN))
      {
      }

      virtual DataFormat::Ptr GetFormat() const
      {
        return Depacker;
      }

      virtual bool Check(const void* data, std::size_t availSize) const
      {
        const CompressorCode4::Container container(data, availSize);
        return container.FastCheck() && Depacker->Match(data, availSize);
      }

      virtual std::auto_ptr<Dump> Decode(const void* data, std::size_t availSize, std::size_t& usedSize) const
      {
        const CompressorCode4::Container container(data, availSize);
        if (!container.FastCheck() || !Depacker->Match(data, availSize))
        {
          return std::auto_ptr<Dump>();
        }
        CompressorCode4::DataDecoder decoder(container);
        if (Dump* decoded = decoder.GetDecodedData())
        {
          usedSize = decoder.GetUsedSize();
          std::auto_ptr<Dump> res(new Dump());
          res->swap(*decoded);
          return res;
        }
        return std::auto_ptr<Dump>();
      }
    private:
      const DataFormat::Ptr Depacker;
    };

    Decoder::Ptr CreateCompressorCode4Decoder()
    {
      return Decoder::Ptr(new CompressorCode4Decoder());
    }
  }
}
