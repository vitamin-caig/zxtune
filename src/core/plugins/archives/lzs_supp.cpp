/*
Abstract:
  LS convertors support

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//local includes
#include <core/plugins/enumerator.h>
#include <core/plugins/utils.h>
//common includes
#include <byteorder.h>
#include <detector.h>
#include <tools.h>
//library includes
#include <core/plugin_attrs.h>
#include <io/container.h>
//std includes
#include <algorithm>
#include <iterator>
//text includes
#include <core/text/plugins.h>

namespace
{
  using namespace ZXTune;

  const Char LZS_PLUGIN_ID[] = {'L', 'Z', 'S', '\0'};
  const String LZS_PLUGIN_VERSION(FromStdString("$Rev$"));

  const std::size_t DEPACKER_SIZE = 0x82;
  const std::string LZS_DEPACKER_PATTERN(
#if 0
      //decoding loop core pattern
      "+38+"
      "06007ecb7f201de60f47"
#else
      "cd??"    // call xxxx
      "?"       // di/nop
      "ed73??"  // ld (xxxx),sp
      "21??"    // ld hl,xxxx
      "11??"    // ld de,xxxx
      "01??"    // ld bc,xxxx
      "d5"      // push de
      "edb0"    // ldir
      "21??"    // ld hl,xxxx
      "11??"    // ld de,xxxx
      "01??"    // ld bc,xxxx ;size of packed. at +x1b
      "c9"      // ret
      //+1e
      "edb8"    // lddr
      "21??"    // ld hl,xxxx
      "11??"    // ld de,xxxx
      "06?"     // ld b,xx (0)
      "7e"      // ld a,(hl)
      "cb7f"    // bit 7,a
      "201d"    // jr nz,...
      "e6?"     // and xx (0xf)
      "47"      // ld b,a
      "ed6f"    // rld
      "c6?"     // add a,xx (3)
      "4f"      // ld c,a
      "23"      // inc hl
      "7b"      // ld a,e
      "96"      // sub (hl)
      "23"      // inc hl
      "f9"      // ld sp,hl
      "66"      // ld h,(hl)
      "6f"      // ld l,a
      "7a"      // ld a,d
      "98"      // sbc a,b
      "44"      // ld b,h
      "67"      // ld h,a
      "78"      // ld a,b
      "06?"     // ld b,xx (0)
      "edb0"    // ldir
      "60"      // ld h,b
      "69"      // ld l,c
      "39"      // add hl,sp
      "18df"    // jr ...
      "e6?"     // and xx (0x7f)
      "2819"    // jr z,...
      "23"      // inc hl
      "cb77"    // bit 6,a
      "2005"    // jr nz,...
      "4f"      // ld c,a
      "edb0"    // ldir
      "18d0"    // jr ...
      "e6?"     // and xx, (0x3f)
      "c6?"     // add a,xx (3)
      "47"      // ld b,a
      "7e"      // ld a,(hl)
      "23"      // inc hl
      "4e"      // ld c,(hl)
      "12"      // ld (de),a
      "13"      // inc de
      "10fc"    // djnz ...
      "79"      // ld a,c
      "18c2"    // jr ...
      "31??"    // ld sp,xxxx
      "06?"     // ld b,xx (3)
      "e1"      // pop hl
      "3b"      // dec sp
      "f1"      // pop af
      "77"      // ld (hl),a
      "10fa"    // djnz ...
      "31??"    // ld sp,xxxx
      "?"       // di/ei
      "c3??"    // jp xxxx (0x0052)
#endif
  );

#ifdef USE_PRAGMA_PACK
#pragma pack(push,1)
#endif
  PACK_PRE struct LZSHeader
  {
    //+0
    char Padding1[0x1b];
    //+1b
    uint16_t SizeOfPacked;
    //+1d
  } PACK_POST;
#ifdef USE_PRAGMA_PACK
#pragma pack(pop)
#endif

  BOOST_STATIC_ASSERT(sizeof(LZSHeader) == 0x1d);

  uint_t GetPackedSize(const LZSHeader& header)
  {
    return DEPACKER_SIZE + fromLE(header.SizeOfPacked);
  }

  bool CheckLZS(const LZSHeader* header, std::size_t limit)
  {
    if (limit < sizeof(*header) ||
        !DetectFormat(safe_ptr_cast<const uint8_t*>(header), limit, LZS_DEPACKER_PATTERN)
        )
    {
      return false;
    }
    const uint_t packed = GetPackedSize(*header);
    return packed <= limit;
  }

  inline bool CopyFromBack(uint_t offset, Dump& dst, uint_t count)
  {
    const std::size_t size = dst.size();
    if (offset > size)
    {
      return false;//invalid backref
    }
    dst.resize(size + count);
    const Dump::iterator dstStart = dst.begin() + size;
    const Dump::const_iterator srcStart = dstStart - offset;
    const Dump::const_iterator srcEnd = srcStart + count;
    RecursiveCopy(srcStart, srcEnd, dstStart);
    return true;
  }

  bool DecodeLZS(const LZSHeader& header, Dump& res)
  {
    const uint_t packedSize = fromLE(header.SizeOfPacked);
    const uint8_t* src = safe_ptr_cast<const uint8_t*>(&header) + DEPACKER_SIZE;
    const uint8_t* const srcEnd = src + packedSize;
    Dump dst;
    dst.reserve(packedSize * 2);//TODO

    while (src != srcEnd)
    {
      const uint_t data = *src++;
      const uint_t code = data & 0xc0;
      if (0x80 == code)
      {
        if (const uint_t len = data & 0x3f)
        {
          if (src + len > srcEnd)
          {
            return false;
          }
          std::copy(src, src + len, std::back_inserter(dst));
          src += len;
        }
        else
        {
          break;
        }
      }
      else if (0xc0 == code)
      {
        const uint_t len = (data & 0x3f) + 3;
        if (src == srcEnd)
        {
          return false;
        }
        std::fill_n(std::back_inserter(dst), len, *src++);
      }
      else
      {
        if (src == srcEnd)
        {
          return false;
        }
        const uint_t len = ((data & 0xf0) >> 4) + 3;
        const uint_t offset = 256 * (data & 0x0f) + *src++;
        if (!CopyFromBack(offset, dst, len))
        {
          return false;
        }
      }
    }
    std::copy(src, srcEnd, std::back_inserter(dst));
    res.swap(dst);
    return true;
  }

  class LZSData
  {
  public:
    LZSData(const uint8_t* data, std::size_t size)
      : Header(0)
    {
      const LZSHeader* const header = safe_ptr_cast<const LZSHeader*>(data);
      if (CheckLZS(header, size))
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
      return DecodeLZS(*Header, res);
    }
  private:
    const LZSHeader* Header;
  };

  class LZSPlugin : public ArchivePlugin
  {
  public:
    virtual String Id() const
    {
      return LZS_PLUGIN_ID;
    }

    virtual String Description() const
    {
      return Text::LZS_PLUGIN_INFO;
    }

    virtual String Version() const
    {
      return LZS_PLUGIN_VERSION;
    }

    virtual uint_t Capabilities() const
    {
      return CAP_STOR_CONTAINER;
    }

    virtual bool Check(const IO::DataContainer& inputData) const
    {
      const uint8_t* const data = static_cast<const uint8_t*>(inputData.Data());
      const std::size_t size = inputData.Size();
      const LZSData lzsData(data, size);
      return lzsData.IsValid();
    }

    virtual IO::DataContainer::Ptr ExtractSubdata(const Parameters::Accessor& /*parameters*/,
      const MetaContainer& input, ModuleRegion& region) const
    {
      const IO::DataContainer& inputData = *input.Data;
      const uint8_t* const data = static_cast<const uint8_t*>(inputData.Data());
      const std::size_t size = inputData.Size();
      const LZSData lzsData(data, size);
      assert(lzsData.IsValid());
      Dump res;
      if (lzsData.Decode(res))
      {
        region.Size = lzsData.PackedSize();
        return IO::CreateDataContainer(res);
      }
      return IO::DataContainer::Ptr();
    }
  };
}

namespace ZXTune
{
  void RegisterLZSConvertor(PluginsEnumerator& enumerator)
  {
    const ArchivePlugin::Ptr plugin(new LZSPlugin());
    enumerator.RegisterPlugin(plugin);
  }
}
