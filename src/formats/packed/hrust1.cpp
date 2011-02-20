/*
Abstract:
  Hrust1 support

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
#include <tools.h>
//library includes
#include <formats/packed.h>
//std includes
#include <numeric>

namespace Hrust1
{
#ifdef USE_PRAGMA_PACK
#pragma pack(push,1)
#endif
  PACK_PRE struct RawHeader
  {
    uint8_t ID[2];//'HR'
    uint16_t DataSize;
    uint16_t PackedSize;
    uint8_t LastBytes[6];
    uint8_t BitStream[2];
    uint8_t ByteStream[1];
  } PACK_POST;
#ifdef USE_PRAGMA_PACK
#pragma pack(pop)
#endif

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
      if (header.ID[0] != 'H' ||
          header.ID[1] != 'R')
      {
        return false;
      }
      return GetUsedSize() <= Size;
    }

    uint_t GetUsedSize() const
    {
      const RawHeader& header = GetHeader();
      return fromLE(header.PackedSize);
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

  typedef Hrust1Bitstream Bitstream;

  inline bool CopyByteFromBack(int_t offset, Dump& dst)
  {
    assert(offset <= 0);
    const std::size_t size = dst.size();
    if (uint_t(-offset) > size)
    {
      return false;//invalid backreference
    }
    const Dump::value_type val = dst[size + offset];
    dst.push_back(val);
    return true;
  }

  inline bool CopyBreaked(int_t offset, Dump& dst, uint8_t data)
  {
    return CopyByteFromBack(offset, dst) && (dst.push_back(data), true) && CopyByteFromBack(offset, dst);
  }

  bool DecodeHrust1(const RawHeader& header, Dump& result)
  {
    const uint_t unpackedSize = fromLE(header.DataSize);
    Dump dst;
    dst.reserve(unpackedSize);

    Bitstream stream(header.BitStream, fromLE(header.PackedSize) - 12);
    //put first byte
    dst.push_back(stream.GetByte());
    uint_t refBits = 2;
    while (!stream.Eof())
    {
      //%1 - put byte
      while (stream.GetBit())
      {
        dst.push_back(stream.GetByte());
      }
      uint_t len = 0;
      for (uint_t bits = 3; bits == 0x3 && len != 0xf;)
      {
         bits = stream.GetBits(2), len += bits;
      }

      //%0 00,-disp3 - copy byte with offset
      if (0 == len)
      {
        const int_t offset = static_cast<int16_t>(0xfff8 + stream.GetBits(3));
        if (!CopyByteFromBack(offset, dst))
        {
          return false;
        }
        continue;
      }
      //%0 01 - copy 2 bytes
      else if (1 == len)
      {
        const uint_t code = stream.GetBits(2);
        int_t offset = 0;
        //%10: -dispH=0xff
        if (2 == code)
        {
          uint_t byte = stream.GetByte();
          if (byte >= 0xe0)
          {
            byte <<= 1;
            ++byte;
            byte ^= 2;
            byte &= 0xff;

            if (byte == 0xff)//inc refsize
            {
              ++refBits;
              continue;
            }
            const int_t offset = static_cast<int16_t>(0xff00 + byte - 0xf);
            if (!CopyBreaked(offset, dst, stream.GetByte()))
            {
              return false;
            }
            continue;
          }
          offset = static_cast<int16_t>(0xff00 + byte);
        }
        //%00..01: -dispH=#fd..#fe,-dispL
        else if (0 == code || 1 == code)
        {
          offset = static_cast<int16_t>((code ? 0xfe00 : 0xfd00) + stream.GetByte());
        }
        //%11,-disp5
        else if (3 == code)
        {
          offset = static_cast<int16_t>(0xffe0 + stream.GetBits(5));
        }
        if (!CopyFromBack(-offset, dst, 2))
        {
          return false;
        }
        continue;
      }
      //%0 1100...
      else if (3 == len)
      {
        //%011001
        if (stream.GetBit())
        {
          const int_t offset = static_cast<int16_t>(0xfff0 + stream.GetBits(4));
          if (!CopyBreaked(offset, dst, stream.GetByte()))
          {
            return false;
          }
          continue;
        }
        //%0110001
        else if (stream.GetBit())
        {
          const uint_t count = 2 * (6 + stream.GetBits(4));
          for (uint_t bytes = 0; bytes < count; ++bytes)
          {
            dst.push_back(stream.GetByte());
          }
          continue;
        }
        else
        {
          len = stream.GetBits(7);
          if (0xf == len)
          {
            break;//EOF
          }
          else if (len < 0xf)
          {
            len = 256 * len + stream.GetByte();
          }
        }
      }

      if (2 == len)
      {
        ++len;
      }
      const uint_t code = stream.GetBits(2);
      int_t offset = 0;
      if (1 == code)
      {
        uint_t byte = stream.GetByte();
        if (byte >= 0xe0)
        {
          if (len > 3)
          {
            return false;
          }
          byte <<= 1;
          ++byte;
          byte ^= 3;
          byte &= 0xff;

          const int_t offset = static_cast<int16_t>(0xff00 + byte - 0xf);
          if (!CopyBreaked(offset, dst, stream.GetByte()))
          {
            return false;
          }
          continue;
        }
        offset = static_cast<int16_t>(0xff00 + byte);
      }
      else if (0 == code)
      {
        offset = static_cast<int16_t>(0xfe00 + stream.GetByte());
      }
      else if (2 == code)
      {
        offset = static_cast<int16_t>(0xffe0 + stream.GetBits(5));
      }
      else if (3 == code)
      {
        static const uint_t Mask[] = {0, 0, 0xfc, 0xf8, 0xf0, 0xe0, 0xc0, 0x80, 0};
        offset = 256 * (Mask[refBits] + stream.GetBits(refBits));
        offset |= stream.GetByte();
        offset = static_cast<int16_t>(offset & 0xffff);
      }
      if (!CopyFromBack(-offset, dst, len))
      {
        return false;
      }
    }
    //put remaining bytes
    std::copy(header.LastBytes, ArrayEnd(header.LastBytes), std::back_inserter(dst));
    result.swap(dst);
    return true;
  }

  class DataDecoder
  {
  public:
    explicit DataDecoder(const Container& container)
      : IsValid(container.FastCheck())
      , Header(container.GetHeader())
    {
    }

    Dump* GetDecodedData()
    {
      if (IsValid && Decoded.empty())
      {
        IsValid = DecodeHrust1(Header, Decoded);
      }
      return IsValid ? &Decoded : 0;
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
    class Hrust1Decoder : public Decoder
    {
    public:
      virtual bool Check(const void* data, std::size_t availSize) const
      {
        const Hrust1::Container container(data, availSize);
        return container.FastCheck();
      }

      virtual std::auto_ptr<Dump> Decode(const void* data, std::size_t availSize, std::size_t& usedSize) const
      {
        const Hrust1::Container container(data, availSize);
        assert(container.FastCheck());
        Hrust1::DataDecoder decoder(container);
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

    Decoder::Ptr CreateHrust1Decoder()
    {
      return Decoder::Ptr(new Hrust1Decoder());
    }
  }
}

