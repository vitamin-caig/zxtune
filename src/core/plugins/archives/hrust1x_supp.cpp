/*
Abstract:
  Hrust 1.x convertors support

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
  (C) Based on XLook sources by HalfElf
*/

//local includes
#include "hrust1_bitstream.h"
#include "pack_utils.h"
#include <core/plugins/detect_helper.h>
#include <core/plugins/enumerator.h>
//common includes
#include <byteorder.h>
#include <tools.h>
//library includes
#include <core/plugin_attrs.h>
#include <io/container.h>
//std includes
#include <numeric>
//text includes
#include <core/text/plugins.h>

#define FILE_TAG 047CB563

namespace
{
  using namespace ZXTune;

  const Char HRUST1X_PLUGIN_ID[] = {'H', 'R', 'U', 'S', 'T', '1', '\0'};
  const String HRUST1X_PLUGIN_VERSION(FromStdString("$Rev$"));

  //checkers
  const DataPrefix DEPACKERS[] =
  {
    {
      "f3"      // di
      "ed73??"  // ld (xxxx),sp
      "11??"    // ld de,xxxx
      "21??"    // ld hl,xxxx
      "01??"    // ld bc,xxxx
      "d5"      // push de
      "edb0"    // ldir
      "13"      // inc de
      "13"      // inc de
      "d5"      // push de
      "dde1"    // pop ix
      "0e?"     // ld c,xx
      "09"      // add hl,bc
      "edb0"    // ldir
      "21??"    // ld hl,xxxx
      "11??"    // ld de,xxxx
      "01??"    // ld bc,xxxx
      "c9"      // ret
      ,
      0x103
    },

    {
      "dd21??"  // ld ix,xxxx
      "dd39"    // add ix,sp
      "d5"      // push de
      "f9"      // ld sp,hl
      "c1"      // pop bc
      "eb"      // ex de,hl
      "c1"      // pop bc
      "0b"      // dec bc
      "09"      // add hl,bc
      "eb"      // ex de,hl
      "c1"      // pop bc
      "0b"      // dec bc
      "09"      // add hl,bc
      "ed52"    // sbc hl,de
      "19"      // add hl,de
      "38?"     // jr c,...
      "54"      // ld d,h
      "5d"      // ld e,l
      "edb8"    // lddr
      "eb"      // ex de,hl
      "dd560b"  // ld d,(ix+0xb)
      "dd5e0a"  // ld e,(ix+0xa)
      "f9"      // ld sp,hl
      "e1"      // pop hl
      "e1"      // pop hl
      "e1"      // pop hl
      "06?"     // ld b,xx (6)
      "3b"      // dec sp
      "f1"      // pop af
      "dd7706"  // ld (ix+6),a
      "dd23"    // inc ix
      "10?"     // djnz xxx, (0xf7)
      ,
      0x100
    }
  };

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

  uint_t GetPackedSize(const Hrust1xHeader& header)
  {
    return fromLE(header.PackedSize);
  }

  uint_t GetUnpackedSize(const Hrust1xHeader& header)
  {
    return fromLE(header.DataSize);
  }

  bool CheckHrust1(const Hrust1xHeader* header, std::size_t size)
  {
    if (size < sizeof(*header) ||
        header->ID[0] != 'H' || header->ID[1] != 'R')
    {
      return false;
    }
    const uint_t packedSize = GetPackedSize(*header);
    const uint_t unpackedSize = GetUnpackedSize(*header);
    return packedSize <= unpackedSize && packedSize <= size;
  }

  bool DecodeHrust1(const Hrust1xHeader& header, Dump& result)
  {
    const uint_t unpackedSize = GetUnpackedSize(header);
    Dump dst;
    dst.reserve(unpackedSize);

    Bitstream stream(header.BitStream, GetPackedSize(header) - 12);
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
    //valid if match
    if (dst.size() != unpackedSize)
    {
      return false;
    }
    result.swap(dst);
    return true;
  }

  class Hrust1Data
  {
  public:
    Hrust1Data(const uint8_t* data, std::size_t size)
      : Header(0)
    {
      const Hrust1xHeader* const header = safe_ptr_cast<const Hrust1xHeader*>(data);
      if (CheckHrust1(header, size))
      {
        Header = header;
      }
    }

    bool IsValid() const
    {
      return Header != 0;
    }

    uint_t PackedSize() const
    {
      assert(Header);
      return GetPackedSize(*Header);
    }

    bool Decode(Dump& res) const
    {
      assert(Header);
      return DecodeHrust1(*Header, res);
    }
  private:
    const Hrust1xHeader* Header;
  };

  //////////////////////////////////////////////////////////////////////////
  class Hrust1xPlugin : public ArchivePlugin
                      , private ArchiveDetector
  {
  public:
    virtual String Id() const
    {
      return HRUST1X_PLUGIN_ID;
    }

    virtual String Description() const
    {
      return Text::HRUST1X_PLUGIN_INFO;
    }

    virtual String Version() const
    {
      return HRUST1X_PLUGIN_VERSION;
    }

    virtual uint_t Capabilities() const
    {
      return CAP_STOR_CONTAINER;
    }

    virtual bool Check(const IO::DataContainer& inputData) const
    {
      return CheckDataFormat(*this, inputData);
    }

    virtual IO::DataContainer::Ptr ExtractSubdata(const Parameters::Accessor& parameters,
      const MetaContainer& input, ModuleRegion& region) const
    {
      return ExtractSubdataFromData(*this, parameters, input, region);
    }
  private:
    virtual bool CheckData(const uint8_t* data, std::size_t size) const
    {
      const Hrust1Data hrustData(data, size);
      return hrustData.IsValid();
    }

    virtual DataPrefixIterator GetPrefixes() const
    {
      return DataPrefixIterator(DEPACKERS, ArrayEnd(DEPACKERS));
    }

    virtual IO::DataContainer::Ptr TryToExtractSubdata(const Parameters::Accessor& /*parameters*/,
      const MetaContainer& container, ModuleRegion& region) const
    {
      const IO::DataContainer& inputData = *container.Data;
      const uint8_t* const data = static_cast<const uint8_t*>(inputData.Data());
      const std::size_t limit = inputData.Size();

      const Hrust1Data hrustData(data + region.Offset, limit - region.Offset);
      Dump res;
      if (hrustData.Decode(res))
      {
        region.Size = hrustData.PackedSize();
        return IO::CreateDataContainer(res);
      }
      return IO::DataContainer::Ptr();
    }
  };
}

namespace ZXTune
{
  void RegisterHrust1xConvertor(PluginsEnumerator& enumerator)
  {
    const ArchivePlugin::Ptr plugin(new Hrust1xPlugin());
    enumerator.RegisterPlugin(plugin);
  }
}
