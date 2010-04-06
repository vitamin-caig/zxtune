/*
Abstract:
  Hrust 1.x implicit convertors support

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#include "../enumerator.h"

#include <byteorder.h>
#include <tools.h>
#include <core/plugin_attrs.h>
#include <io/container.h>

#include <numeric>

#include <text/plugins.h>

#define FILE_TAG 047CB563

namespace
{
  using namespace ZXTune;

  const Char HRUST1X_PLUGIN_ID[] = {'H', 'R', 'U', 'S', 'T', '1', '\0'};
  const String TEXT_HRUST1X_VERSION(FromStdString("$Rev$"));

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

  //hrust1x bitstream decoder
  class Bitstream
  {
  public:
    Bitstream(const uint8_t* data, std::size_t size)
      : Data(data), End(Data + size), Bits(), Mask(0x8000)
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

    uint_t GetBit()
    {
      const uint_t result = (Bits & Mask) != 0 ? 1 : 0;
      if (!(Mask >>= 1))
      {
        Bits = GetByte();
        Bits |= 256 * GetByte();
        Mask = 0x8000;
      }
      return result;
    }

    uint_t GetBits(unsigned count)
    {
      uint_t result = 0;
      while (count--)
      {
        result = 2 * result | GetBit();
      }
      return result;
    }
  private:
    const uint8_t* Data;
    const uint8_t* const End;
    uint_t Bits;
    uint_t Mask;
  };

  inline bool CopyFromBack(int_t offset, Dump& dst)
  {
    if (offset < -static_cast<int_t>(dst.size()))
    {
      return false;//invalid backreference
    }
    dst.push_back(*(dst.end() + offset));
    return true;
  }

  inline bool CopyFromBack(int_t offset, Dump& dst, uint_t count)
  {
    if (offset < -static_cast<int_t>(dst.size()))
    {
      return false;//invalid backref
    }
    std::copy(dst.end() + offset, dst.end() + offset + count, std::back_inserter(dst));
    return true;
  }

  inline bool CopyBreaked(int_t offset, Dump& dst, uint8_t data)
  {
    return CopyFromBack(offset, dst) && (dst.push_back(data), true) && CopyFromBack(offset, dst);
  }

  bool Decode(const Hrust1xHeader* header, std::size_t size, Dump& result)
  {
    if (size < sizeof(*header) ||
        header->ID[0] != 'H' || header->ID[1] != 'R' ||
        fromLE(header->PackedSize) > fromLE(header->DataSize) ||
        fromLE(header->PackedSize) > size)
    {
      return false;
    }

    Dump dst;
    dst.reserve(fromLE(header->DataSize));

    Bitstream stream(header->BitStream, fromLE(header->PackedSize) - 12);
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
        if (!CopyFromBack(offset, dst))
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
      if (!CopyFromBack(offset, dst, len))
      {
        return false;
      }
    }
    //put remaining bytes
    std::copy(header->LastBytes, ArrayEnd(header->LastBytes), std::back_inserter(dst));
    //valid if match
    if (dst.size() == fromLE(header->DataSize))
    {
      result.swap(dst);
      return true;
    }
    return false;
  }

  //////////////////////////////////////////////////////////////////////////
  bool ProcessHrust1x(const Parameters::Map& /*commonParams*/, const MetaContainer& input,
    IO::DataContainer::Ptr& output, ModuleRegion& region)
  {
    const IO::DataContainer& inputData = *input.Data;
    const std::size_t limit = inputData.Size();
    const Hrust1xHeader* const header1 = static_cast<const Hrust1xHeader*>(inputData.Data());
    Dump res;
    //check without depacker
    if (Decode(header1, limit, res))
    {
      output = IO::CreateDataContainer(res);
      region.Offset = 0;
      region.Size = fromLE(header1->PackedSize);
      return true;
    }
    //check with depacker
    const Hrust1xHeader* const header2 = safe_ptr_cast<const Hrust1xHeader*>(static_cast<const uint8_t*>(inputData.Data()) +
      DEPACKER_SIZE);
    if (limit > DEPACKER_SIZE &&
      Decode(header2, limit - DEPACKER_SIZE, res))
    {
      output = IO::CreateDataContainer(res);
      region.Offset = DEPACKER_SIZE;
      region.Size = fromLE(header2->PackedSize);
      return true;
    }
    return false;
  }
}

namespace ZXTune
{
  void RegisterHrust1xConvertor(PluginsEnumerator& enumerator)
  {
    PluginInformation info;
    info.Id = HRUST1X_PLUGIN_ID;
    info.Description = TEXT_HRUST1X_INFO;
    info.Version = TEXT_HRUST1X_VERSION;
    info.Capabilities = CAP_STOR_CONTAINER;
    enumerator.RegisterImplicitPlugin(info, ProcessHrust1x);
  }
}
