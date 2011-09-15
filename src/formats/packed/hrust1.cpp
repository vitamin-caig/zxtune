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
//boost includes
#include <boost/make_shared.hpp>

namespace Hrust1
{
  const std::size_t MAX_DECODED_SIZE = 0xc000;

  const uint8_t SIGNATURE[] = {'H', 'R'};

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
      if (Size <= sizeof(RawHeader))
      {
        return false;
      }
      const RawHeader& header = GetHeader();
      if (header.ID[0] != SIGNATURE[0] ||
          header.ID[1] != SIGNATURE[1])
      {
        return false;
      }
      const std::size_t usedSize = GetUsedSize();
      return in_range(usedSize, sizeof(header), Size);
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

  class DataDecoder
  {
  public:
    explicit DataDecoder(const Container& container)
      : IsValid(container.FastCheck())
      , Header(container.GetHeader())
      , Stream(Header.BitStream, container.GetUsedSize() - 12)
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
  private:
    bool DecodeData()
    {
      Decoded.reserve(fromLE(Header.DataSize));

      //put first byte
      Decoded.push_back(Stream.GetByte());
      uint_t refBits = 2;
      while (!Stream.Eof() && Decoded.size() < MAX_DECODED_SIZE)
      {
        //%1 - put byte
        if (Stream.GetBit())
        {
          Decoded.push_back(Stream.GetByte());
          continue;
        }
        uint_t len = Stream.GetLen();

        //%0 00,-disp3 - copy byte with offset
        if (0 == len)
        {
          const int_t offset = static_cast<int16_t>(0xfff8 + Stream.GetBits(3));
          if (!CopyByteFromBack(offset))
          {
            return false;
          }
          continue;
        }
        //%0 01 - copy 2 bytes
        else if (1 == len)
        {
          const uint_t code = Stream.GetBits(2);
          int_t offset = 0;
          //%10: -dispH=0xff
          if (2 == code)
          {
            uint_t byte = Stream.GetByte();
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
              if (!CopyBreaked(offset))
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
            offset = static_cast<int16_t>((code ? 0xfe00 : 0xfd00) + Stream.GetByte());
          }
          //%11,-disp5
          else if (3 == code)
          {
            offset = static_cast<int16_t>(0xffe0 + Stream.GetBits(5));
          }
          if (!CopyFromBack(-offset, Decoded, 2))
          {
            return false;
          }
          continue;
        }
        //%0 1100...
        else if (3 == len)
        {
          //%011001
          if (Stream.GetBit())
          {
            const int_t offset = static_cast<int16_t>(0xfff0 + Stream.GetBits(4));
            if (!CopyBreaked(offset))
            {
              return false;
            }
            continue;
          }
          //%0110001
          else if (Stream.GetBit())
          {
            const uint_t count = 2 * (6 + Stream.GetBits(4));
            for (uint_t bytes = 0; bytes < count; ++bytes)
            {
              Decoded.push_back(Stream.GetByte());
            }
            continue;
          }
          else
          {
            len = Stream.GetBits(7);
            if (0xf == len)
            {
              break;//EOF
            }
            else if (len < 0xf)
            {
              len = 256 * len + Stream.GetByte();
            }
          }
        }

        if (2 == len)
        {
          ++len;
        }
        const uint_t code = Stream.GetBits(2);
        int_t offset = 0;
        if (1 == code)
        {
          uint_t byte = Stream.GetByte();
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
            if (!CopyBreaked(offset))
            {
              return false;
            }
            continue;
          }
          offset = static_cast<int16_t>(0xff00 + byte);
        }
        else if (0 == code)
        {
          offset = static_cast<int16_t>(0xfe00 + Stream.GetByte());
        }
        else if (2 == code)
        {
          offset = static_cast<int16_t>(0xffe0 + Stream.GetBits(5));
        }
        else if (3 == code)
        {
          static const uint_t Mask[] = {0, 0, 0xfc, 0xf8, 0xf0, 0xe0, 0xc0, 0x80, 0};
          offset = 256 * (Mask[refBits] + Stream.GetBits(refBits));
          offset |= Stream.GetByte();
          offset = static_cast<int16_t>(offset & 0xffff);
        }
        if (!CopyFromBack(-offset, Decoded, len))
        {
          return false;
        }
      }
      //put remaining bytes
      std::copy(Header.LastBytes, ArrayEnd(Header.LastBytes), std::back_inserter(Decoded));
      return true;
    }

    bool CopyByteFromBack(int_t offset)
    {
      assert(offset <= 0);
      const std::size_t size = Decoded.size();
      if (uint_t(-offset) > size)
      {
        return false;//invalid backreference
      }
      const Dump::value_type val = Decoded[size + offset];
      Decoded.push_back(val);
      return true;
    }

    bool CopyBreaked(int_t offset)
    {
      return CopyByteFromBack(offset) && 
             (Decoded.push_back(Stream.GetByte()), true) && 
             CopyByteFromBack(offset);
    }
  private:
    bool IsValid;
    const RawHeader& Header;
    Hrust1Bitstream Stream;
    Dump Decoded;
  };

  class Format : public Binary::Format
  {
  public:
    virtual bool Match(const void* data, std::size_t size) const
    {
      if (ArraySize(SIGNATURE) > size)
      {
        return false;
      }
      return std::equal(SIGNATURE, ArrayEnd(SIGNATURE), static_cast<const uint8_t*>(data));
    }

    virtual std::size_t Search(const void* data, std::size_t size) const
    {
      if (ArraySize(SIGNATURE) > size)
      {
        return size;
      }
      const uint8_t* const rawData = static_cast<const uint8_t*>(data);
      return std::search(rawData, rawData + size, SIGNATURE, ArrayEnd(SIGNATURE)) - rawData;
    }
  };
}

namespace Formats
{
  namespace Packed
  {
    class Hrust1Decoder : public Decoder
    {
    public:
      virtual Binary::Format::Ptr GetFormat() const
      {
        return boost::make_shared<Hrust1::Format>();
      }

      virtual bool Check(const void* data, std::size_t availSize) const
      {
        const Hrust1::Container container(data, availSize);
        return container.FastCheck();
      }

      virtual std::auto_ptr<Dump> Decode(const void* data, std::size_t availSize, std::size_t& usedSize) const
      {
        const Hrust1::Container container(data, availSize);
        if (!container.FastCheck())
        {
          return std::auto_ptr<Dump>();
        }
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
      return boost::make_shared<Hrust1Decoder>();
    }
  }
}

