/*
Abstract:
  Powerfull Code Decreaser convertors support

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
  (C) Based on XLook sources by HalfElf
*/

//local includes
#include "hrust1_bitstream.h"
#include "pack_utils.h"
#include <core/plugins/enumerator.h>
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

  const Char PCD_PLUGIN_ID[] = {'P', 'C', 'D', '\0'};
  const String PCD_PLUGIN_VERSION(FromStdString("$Rev$"));

  const std::string DEPACKER_6_1 = 
      "f3"      // di
      "21??"    // ld hl,xxxx 0xc017   depacker src
      "11??"    // ld de,xxxx 0xbf00   depacker target
      "01b500"  // ld bc,xxxx 0x00b5   depacker size
      "d5"      // push de
      "edb0"    // ldir
      "21??"    // ld hl,xxxx 0xd845   last of src packed (data = +0xe)
      "11??"    // ld de,xxxx 0xffff   last of dst packed (data = +0x11)
      "01??"    // ld bc,xxxx 0x177d   packed size (data = +0x14)
      "c9"      // ret
      "edb8"    // lddr                +0x17
      "21??"    // ld hl,xxxx 0xe883   rest bytes src (data = +0x1a)
      "11??"    // ld de,xxxx 0xbfb2   rest bytes dst (data = +0x1d)
      "01??"    // ld bc,xxxx 0x0003   rest bytes count
      "d5"      // push de
      "c5"      // push bc
      "edb0"    // ldir
      "ed73??"  // ld (xxxx),sp 0xbfa3
      "f9"      // ld sp,hl
      "11??"    // ld de,xxxx   0xc000 target of depack (data = +0x2c)
      "60"      // ld h,b
      "d9"      // exx
      "011001"  // ld bc,xxxx ; 0x110
      "3ed9"    // ld a,x ;0xd9
      "1002"    // djnz xxx, (2)
      "e1"      // pop hl
      "41"      // ld b,c
      "29"      // add hl,hl
      "3007"    // jr nc,xx
      "3b"      // dec sp
      "f1"      // pop af
      "d9"      // exx
      "12"      // ld (de),a
      "13"      // inc de
      "18f1"    // jr ...
  ;

  const std::string DEPACKER_6_2 =
      "f3"
      "21??"    // ld hl,xxxx 0x6026   depacker src
      "11??"    // ld de,xxxx 0x5b00   depacker target
      "01a300"  // ld bc,xxxx 0x00a3   depacker size
      "edb0"    // ldir
      "011001"  // ld bc,xxxx ; 0x110
      "d9"      // exx
      "22??"    // ld (xxxx),hl 0x5b97
      "21??"    // ld hl,xxxx 0xb244   last of src packed (data = +0x14)
      "11??"    // ld de,xxxx 0xfaa4   last of dst packed (data = +0x17)
      "01??"    // ld bc,xxxx 0x517c   packed size (data = +0x1a)
      "ed73??"  // ld (xxxx),sp 0x0000
      "31??"    // ld sp,xxxx   0xa929
      "c3??"    // jp xxxx 0x5b00
      "edb8"    // lddr                +0x26
      "11??"    // ld de,xxxx 0x6000   target of depack (data = +0x29)
      "60"      // ld h,b
      "d9"      // exx
      "1002"    // djnz xxx, (2)
      "e1"      // pop hl
      "41"      // ld b,c
      "29"      // add hl,hl
      "3007"    // jr nc,xx
      "3b"      // dec sp
      "f1"      // pop af
      "d9"      // exx
      "12"      // ld (de),a
      "13"      // inc de
      "18f1"    // jr ...
  ;


#ifdef USE_PRAGMA_PACK
#pragma pack(push,1)
#endif
  PACK_PRE struct PCD61Depacker
  {
    //+0
    uint8_t Padding1[0xe];
    //+0xe
    uint16_t LastOfSrcPacked;
    //+0x10
    uint8_t Padding2;
    //+0x11
    uint16_t LastOfDstPacked;
    //+0x13
    uint8_t Padding3;
    //+0x14
    uint16_t PackedSize;
    //+0x16
    uint8_t Padding4[0xb3];
    //+0xc9
    uint8_t LastBytes[3];
    uint8_t Bitstream[2];
  } PACK_POST;

  PACK_PRE struct PCD62Depacker
  {
    //+0
    uint8_t Padding1[0x14];
    //+0x14
    uint16_t LastOfSrcPacked;
    //+0x16
    uint8_t Padding2;
    //+0x17
    uint16_t LastOfDstPacked;
    //+0x19
    uint8_t Padding3;
    //+0x1a
    uint16_t PackedSize;
    //+0x1c
    uint8_t Padding4[0xa8];
    //+0xc4
    uint8_t LastBytes[5];
    uint8_t Bitstream[2];
  } PACK_POST;
#ifdef USE_PRAGMA_PACK
#pragma pack(pop)
#endif

  BOOST_STATIC_ASSERT(sizeof(PCD61Depacker) == 0xc9 + 5);
  BOOST_STATIC_ASSERT(sizeof(PCD62Depacker) == 0xc4 + 7);

  class Bitstream
  {
  public:
    Bitstream(const uint8_t* data, std::size_t size)
      : Data(data), End(Data + size), Bits(), Mask(0)
    {
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
      if (!(Mask >>= 1))
      {
        Bits = GetByte();
        Bits |= 256 * GetByte();
        Mask = 0x8000;
      }
      return (Bits & Mask) != 0 ? 1 : 0;
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

  template<class Header>
  uint_t GetPackedSize(const Header& header)
  {
    return sizeof(Header) + fromLE(header.PackedSize) - sizeof(header.LastBytes) - sizeof(header.Bitstream);
  }

  template<class Header>
  bool FastCheckPCD(const Header* header, std::size_t limit)
  {
    if (limit < sizeof(*header))
    {
      return false;
    }
    const int_t packedDataMoveDelta = fromLE(header->LastOfDstPacked) - fromLE(header->LastOfSrcPacked);
    //cannot overlap previously packed area
    if (packedDataMoveDelta < 0 &&
        uint_t(-packedDataMoveDelta) < fromLE(header->PackedSize) + sizeof(header->LastBytes))
    {
      return false;
    }
    const uint_t packed = GetPackedSize(*header);
    return packed <= limit;
  }

  template<class Header>
  bool DecodePCD(const Header& header, Dump& res)
  {
    const uint_t packedSize = fromLE(header.PackedSize) - sizeof(header.LastBytes);
    Dump dst;
    dst.reserve(packedSize * 2);//TODO

    Bitstream stream(header.Bitstream, packedSize);
    while (!stream.Eof())
    {
      while (stream.GetBit())
      {
        dst.push_back(stream.GetByte());
      }
      uint_t len = 0;
      for (uint_t bits = 3; bits == 0x3 && len != 0xf;)
      {
        bits = stream.GetBits(2);
        len += bits;
      }
      if (0 == len)
      {
        const uint_t offset = stream.GetBits(4) + 1;
        if (!CopyFromBack(offset, dst, 1))
        {
          return false;
        }
        continue;
      }
      else if (1 == len)
      {
        const uint_t offset = stream.GetByte();
        if (0xff == offset)
        {
          break;
        }
        if (!CopyFromBack(offset + 1, dst, 2))
        {
          return false;
        }
        continue;
      }
      else if (3 == len)
      {
        len = stream.GetByte();
        if (0 == len)
        {
          len = stream.GetByte();
          len += 256 * stream.GetByte();
        }
        else
        {
          len += 0xf;
        }
      }
      if (2 == len)
      {
         len = 3;
      }
      const uint_t loOffset = stream.GetByte();
      uint_t hiOffset = 0;
      if (stream.GetBit())
      {
        do
        {
          hiOffset = 4 * hiOffset + stream.GetBits(2);
        }
        while (stream.GetBit());
        ++hiOffset;
      }
      const uint_t offset = 256 * hiOffset + loOffset;
      if (!CopyFromBack(offset, dst, len))
      {
        return false;
      }
    }
    std::copy(header.LastBytes, ArrayEnd(header.LastBytes), std::back_inserter(dst));
    res.swap(dst);
    return true;
  }

  class PCDData
  {
  public:
    PCDData(const uint8_t* data, std::size_t size)
      : Header1(0)
      , Header2(0)
    {
      const PCD61Depacker* const header1 = safe_ptr_cast<const PCD61Depacker*>(data);
      const PCD62Depacker* const header2 = safe_ptr_cast<const PCD62Depacker*>(data);
      if (FastCheckPCD(header1, size) &&
          DetectFormat(data, size, DEPACKER_6_1))
      {
        Header1 = header1;
      }
      else if (FastCheckPCD(header2, size) &&
          DetectFormat(data, size, DEPACKER_6_2))
      {
        Header2 = header2;
      }
    }

    bool IsValid() const
    {
      return Header1 != 0 || Header2 != 0;
    }

    uint_t PackedSize() const
    {
      assert(Header1 || Header2);
      return Header1
        ? GetPackedSize(*Header1)
        : GetPackedSize(*Header2);
    }

    bool Decode(Dump& res) const
    {
      assert(Header1 || Header2);
      return Header1
        ? DecodePCD(*Header1, res)
        : DecodePCD(*Header2, res);
    }
  private:
    const PCD61Depacker* Header1;
    const PCD62Depacker* Header2;
  };

  class PCDPlugin : public ArchivePlugin
  {
  public:
    virtual String Id() const
    {
      return PCD_PLUGIN_ID;
    }

    virtual String Description() const
    {
      return Text::PCD_PLUGIN_INFO;
    }

    virtual String Version() const
    {
      return PCD_PLUGIN_VERSION;
    }

    virtual uint_t Capabilities() const
    {
      return CAP_STOR_CONTAINER;
    }

    virtual bool Check(const IO::DataContainer& inputData) const
    {
      const uint8_t* const data = static_cast<const uint8_t*>(inputData.Data());
      const std::size_t size = inputData.Size();
      const PCDData pcdData(data, size);
      return pcdData.IsValid();
    }

    virtual IO::DataContainer::Ptr ExtractSubdata(const Parameters::Accessor& /*parameters*/,
      const MetaContainer& input, ModuleRegion& region) const
    {
      const IO::DataContainer& inputData = *input.Data;
      const uint8_t* const data = static_cast<const uint8_t*>(inputData.Data());
      const std::size_t size = inputData.Size();
      const PCDData pcdData(data, size);
      assert(pcdData.IsValid());
      Dump res;
      if (pcdData.Decode(res))
      {
        region.Size = pcdData.PackedSize();
        return IO::CreateDataContainer(res);
      }
      return IO::DataContainer::Ptr();
    }
  };
}

namespace ZXTune
{
  void RegisterPCDConvertor(PluginsEnumerator& enumerator)
  {
    const ArchivePlugin::Ptr plugin(new PCDPlugin());
    enumerator.RegisterPlugin(plugin);
  }
}
