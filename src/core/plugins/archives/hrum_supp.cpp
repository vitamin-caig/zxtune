/*
Abstract:
  Hrum convertors support

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
#include <detector.h>
#include <tools.h>
//library includes
#include <core/plugin_attrs.h>
#include <io/container.h>
//std includes
#include <numeric>
//text includes
#include <core/text/plugins.h>

namespace
{
  using namespace ZXTune;

  const Char HRUM_PLUGIN_ID[] = {'H', 'R', 'U', 'M', '\0'};
  const String HRUM_PLUGIN_VERSION(FromStdString("$Rev$"));

  //checkers
  const std::string HRUM_DEPACKER =
      "?"       // di/nop
      "ed73??"  // ld (xxxx),sp
      "21??"    // ld hl,xxxx   start+0x1f
      "11??"    // ld de,xxxx   tmp buffer
      "017700"  // ld bc,0x0077 size of depacker
      "d5"      // push de
      "edb0"    // ldir
      "11??"    // ld de,xxxx   dst of depack (data = +0x12)
      "d9"      // exx
      "21??"    // ld hl,xxxx   last byte of src packed (data = +0x16)
      "11??"    // ld de,xxxx   last byte of dst packed (data = +0x19)
      "01??"    // ld bc,xxxx   size of packed          (data = +0x1c)
      "c9"      // ret
      "ed?"     // lddr/ldir
  ;

#ifdef USE_PRAGMA_PACK
#pragma pack(push,1)
#endif
  PACK_PRE struct HrumDepacker
  {
    //+0
    uint8_t Padding1[0x12];
    //+0x12
    uint16_t DepackAddress;
    //+0x14
    uint8_t Padding2[2];
    //+0x16
    uint16_t PackedSource;
    //+0x18
    uint8_t Padding3;
    //+0x19
    uint16_t PackedTarget;
    //+0x1b
    uint8_t Padding4;
    //+0x1c
    uint16_t SizeOfPacked;
    //+0x1e
    uint8_t Padding5[2];
    //+0x20
    uint8_t PackedCopyDirection;
    //+0x21
    uint8_t Padding6[0x70];
    //+0x91
    uint8_t LastBytes[5];
    //+0x96 taken from stack to initialize variables, always 0x1010
    //packed data starts from here
    uint8_t Padding7[2];
    //+0x98
    uint8_t BitStream[2];
    //+0x9a
    uint8_t ByteStream[1];
    //+0x9b
  } PACK_POST;
#ifdef USE_PRAGMA_PACK
#pragma pack(pop)
#endif

  BOOST_STATIC_ASSERT(sizeof(HrumDepacker) == 0x9b);

  class Bitstream : public Hrust1Bitstream
  {
  public:
    Bitstream(const uint8_t* data, std::size_t size)
      : Hrust1Bitstream(data, size)
    {
    }

    uint_t GetDist()
    {
      if (GetBit())
      {
        const uint_t hiBits = 0xf0 + GetBits(4);
        return 256 * hiBits + GetByte();
      }
      else
      {
        return 0xff00 + GetByte();
      }
    }
  };

  uint_t GetPackedSize(const HrumDepacker& header)
  {
    return sizeof(header) - 5/*last 3 members of header*/
      + fromLE(header.SizeOfPacked);
  }

  uint_t GetUnpackedSize(const HrumDepacker& header)
  {
    if (header.PackedCopyDirection == 0xb8)//lddr
    {
      return fromLE(header.PackedTarget) + 1 - fromLE(header.DepackAddress);
    }
    else //ldir
    {
      assert(header.PackedCopyDirection == 0xb0);
      return fromLE(header.PackedTarget) + fromLE(header.SizeOfPacked) - fromLE(header.DepackAddress);
    }
  }

  bool CheckHrum(const HrumDepacker* header, std::size_t size)
  {
    if (size < sizeof(*header))
    {
      return false;
    }
    switch (header->PackedCopyDirection)
    {
    case 0xb8:
    case 0xb0:
      if (fromLE(header->PackedTarget) <= fromLE(header->DepackAddress))
      {
        return false;
      }
      break;
    default:
      return false;
    }
    const uint_t packedSize = GetPackedSize(*header);
    const uint_t unpackedSize = GetUnpackedSize(*header);
    return packedSize <= unpackedSize && packedSize <= size;
  }

  bool DecodeHrum(const HrumDepacker& header, Dump& result)
  {
    const uint_t unpackedSize = GetUnpackedSize(header);
    Dump dst;
    dst.reserve(unpackedSize);

    Bitstream stream(header.BitStream, fromLE(header.SizeOfPacked) - 2);
    //put first byte
    dst.push_back(stream.GetByte());
    while (!stream.Eof())
    {
      while (stream.GetBit())
      {
        dst.push_back(stream.GetByte());
      }
      uint_t len = 1;
      for (uint_t bits = 3; bits == 0x3 && len != 0x10;)
      {
         bits = stream.GetBits(2);
         len += bits;
      }
      uint_t offset = 0;
      if (4 == len)
      {
        len = stream.GetByte();
        if (!len)
        {
          //eof
          break;
        }
        offset = stream.GetDist();
      }
      else
      {
        if (len > 4)
        {
          --len;
        }
        if (1 == len)
        {
          offset = 0xfff8 + stream.GetBits(3);
        }
        else
        {
          if (2 == len)
          {
            offset = 0xff00 + stream.GetByte();
          }
          else
          {
            offset = stream.GetDist();
          }
        }
      }
      if (!CopyFromBack(-static_cast<int16_t>(offset), dst, len))
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

  class HrumData
  {
  public:
    HrumData(const uint8_t* data, std::size_t size)
      : Header(0)
    {
      const HrumDepacker* const header = safe_ptr_cast<const HrumDepacker*>(data);
      if (CheckHrum(header, size) &&
          DetectFormat(data, size, HRUM_DEPACKER))
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
      return DecodeHrum(*Header, res);
    }
  private:
    const HrumDepacker* Header;
  };

  //////////////////////////////////////////////////////////////////////////
  class HrumPlugin : public ArchivePlugin
  {
  public:
    virtual String Id() const
    {
      return HRUM_PLUGIN_ID;
    }

    virtual String Description() const
    {
      return Text::HRUM_PLUGIN_INFO;
    }

    virtual String Version() const
    {
      return HRUM_PLUGIN_VERSION;
    }

    virtual uint_t Capabilities() const
    {
      return CAP_STOR_CONTAINER;
    }

    virtual bool Check(const IO::DataContainer& inputData) const
    {
      const uint8_t* const data = static_cast<const uint8_t*>(inputData.Data());
      const std::size_t size = inputData.Size();
      const HrumData hrumData(data, size);
      return hrumData.IsValid();
    }

    virtual IO::DataContainer::Ptr ExtractSubdata(const Parameters::Accessor& /*parameters*/,
      const IO::DataContainer& data, ModuleRegion& region) const
    {
      const HrumData hrumData(static_cast<const uint8_t*>(data.Data()), data.Size());
      assert(hrumData.IsValid());
      Dump res;
      if (hrumData.Decode(res))
      {
        region.Offset = 0;
        region.Size = hrumData.PackedSize();
        return IO::CreateDataContainer(res);
      }
      return IO::DataContainer::Ptr();
    }
  };
}

namespace ZXTune
{
  void RegisterHrumConvertor(PluginsEnumerator& enumerator)
  {
    const ArchivePlugin::Ptr plugin(new HrumPlugin());
    enumerator.RegisterPlugin(plugin);
  }
}
