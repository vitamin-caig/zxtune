#include "formats_enumerator.h"

#include <types.h>
#include <tools.h>

namespace
{
  using namespace ZXTune::Archive;

  const String::value_type HRUST_1X_ID[] = {'H', 'r', 'u', 's', 't', '1', '.', 'x', 0};

  const std::size_t DEPACKER_SIZE = 0x103;

#ifdef USE_PRAGMA_PACK
#pragma pack(push,1)
#endif
  PACK_PRE struct Hrust1xHeader
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

  class Bitstream
  {
  public:
    Bitstream(const void* data, std::size_t size)
      : Data(static_cast<const uint8_t*>(data)), End(Data + size), Bits(), Mask(0x8000)
    {
      Bits = GetByte();
      Bits |= 256 * GetByte();
    }

    bool Eof() const
    {
      return Data >= End;
    }

    uint8_t GetByte()
    {
      return Eof() ? 0 : *Data++;
    }

    uint8_t GetBit()
    {
      const uint8_t result(Bits & Mask ? 1 : 0);
      if (!(Mask >>= 1))
      {
        Bits = GetByte();
        Bits |= 256 * GetByte();
        Mask = 0x8000;
      }
      return result;
    }

    uint8_t GetBits(unsigned count)
    {
      uint8_t result(0);
      while (count--)
      {
        result = 2 * result | GetBit();
      }
      return result;
    }
  private:
    const uint8_t* Data;
    const uint8_t* const End;
    unsigned Bits;
    unsigned Mask;
  };

  inline bool CopyFromBack(signed offset, Dump& dst)
  {
    if (offset < -signed(dst.size()))
    {
      return false;//invalid backreference
    }
    dst.push_back(*(dst.end() + offset));
    return true;
  }

  inline bool CopyFromBack(signed offset, Dump& dst, unsigned count)
  {
    if (offset < -signed(dst.size()))
    {
      return false;//invalid backref
    }
    std::copy(dst.end() + offset, dst.end() + offset + count, std::back_inserter(dst));
    return true;
  }

  inline bool CopyBreaked(signed offset, Dump& dst, uint8_t data)
  {
    return CopyFromBack(offset, dst) && (dst.push_back(data), true) && CopyFromBack(offset, dst);
  }

  class Decoder
  {
  public:
    explicit Decoder(const Hrust1xHeader* header) : Header(header)
    {
    }

    bool Process(Dump& dst)
    {
      dst.reserve(fromLE(Header->DataSize));

      Bitstream stream(Header->BitStream, fromLE(Header->PackedSize) - 12);
      //put first byte
      dst.push_back(stream.GetByte());
      unsigned refBits(2);
      while (!stream.Eof())
      {
        //%1 - put byte
        while (stream.GetBit())
        {
          dst.push_back(stream.GetByte());
        }
        unsigned len(0);
        for (unsigned bits = 3; bits == 0x3 && len != 0xf;)
        {
           bits = stream.GetBits(2), len += bits;
        }

        //%0 00,-disp3 - copy byte with offset
        if (0 == len)
        {
          const int16_t offset(static_cast<int16_t>(0xfff8 + stream.GetBits(3)));
          if (!CopyFromBack(offset, dst))
          {
            return false;
          }
          continue;
        }
        //%0 01 - copy 2 bytes
        else if (1 == len)
        {
          const unsigned code(stream.GetBits(2));
          signed offset(0);
          //%10: -dispH=0xff
          if (2 == code)
          {
            uint8_t byte(stream.GetByte());
            if (byte >= 0xe0)
            {
              byte <<= 1;
              ++byte;
              byte ^= 2;

              if (byte == 0xff)//inc refsize
              {
                ++refBits;
                continue;
              }
              const signed offset(static_cast<int16_t>(0xff00 + byte - 0xf));
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
          if (!CopyFromBack(offset, dst, 2))
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
            const signed offset(static_cast<int16_t>(0xfff0 + stream.GetBits(4)));
            if (!CopyBreaked(offset, dst, stream.GetByte()))
            {
              return false;
            }
            continue;
          }
          //%0110001
          else if (stream.GetBit())
          {
            const unsigned count(2 * (6 + stream.GetBits(4)));
            for (unsigned bytes = 0; bytes < count; ++bytes)
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
        const unsigned code(stream.GetBits(2));
        signed offset(0);
        if (1 == code)
        {
          uint8_t byte(stream.GetByte());
          if (byte >= 0xe0)
          {
            if (len > 3)
            {
              return false;
            }
            byte <<= 1;
            ++byte;
            byte ^= 3;

            const signed offset(static_cast<int16_t>(0xff00 + byte - 0xf));
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
          static const unsigned Mask[] = {0, 0, 0xfc, 0xf8, 0xf0, 0xe0, 0xc0, 0x80, 0};
          offset = 256 * (Mask[refBits] + stream.GetBits(refBits));
          offset |= stream.GetByte();
          offset = static_cast<int16_t>(offset);
        }
        if (!CopyFromBack(offset, dst, len))
        {
          return false;
        }
      }
      //put remaining bytes
      std::copy(Header->LastBytes, ArrayEnd(Header->LastBytes), std::back_inserter(dst));
      return dst.size() == fromLE(Header->DataSize);//valid if match
    }
  private:
    const Hrust1xHeader* const Header;
  };

  bool DoCheck(const void* data, std::size_t size)
  {
    const Hrust1xHeader* const header(static_cast<const Hrust1xHeader*>(data));
    return (size >= sizeof(*header) && 
      header->ID[0] == 'H' && header->ID[1] == 'R' &&
      fromLE(header->PackedSize) <= fromLE(header->DataSize) &&
      fromLE(header->PackedSize) <= size);
  }

  bool Checker(const void* data, std::size_t size)
  {
    return DoCheck(data, size) || 
      (size >= DEPACKER_SIZE && DoCheck(DEPACKER_SIZE + static_cast<const uint8_t*>(data), 
        size - DEPACKER_SIZE));
  }

  bool Depacker(const void* data, std::size_t size, Dump& dst)
  {
    assert(Checker(data, size) || !"Attempt to unpack non-checked data");
    const Hrust1xHeader* const header1(static_cast<const Hrust1xHeader*>(data));
    const Hrust1xHeader* const header2(safe_ptr_cast<const Hrust1xHeader*>(static_cast<const uint8_t*>(data) + 
      DEPACKER_SIZE));
    Decoder decoder(header1->ID[0] == 'H' && header1->ID[1] == 'R' ? header1 : header2);
    return decoder.Process(dst);
  }

  AutoFormatRegistrator autoReg(HRUST_1X_ID, Checker, Depacker);
}
