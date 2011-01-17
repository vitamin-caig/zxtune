/*
Abstract:
  MsPack convertors support

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
  (C) Based on XLook sources by HalfElf
*/

//local includes
#include <core/plugins/detect_helper.h>
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

  const Char MSP_PLUGIN_ID[] = {'M', 'S', 'P', '\0'};
  const String MSP_PLUGIN_VERSION(FromStdString("$Rev$"));

  const char MSP_SIGNATURE[] = {'M', 's', 'P', 'k'};

  //checkers
  const DataPrefix DEPACKERS[] =
  {
    {
      "f3"      // di
      "ed73??"  // ld (xxxx),sp
      "d9"      // exx
      "22??"    // ld (xxxx),hl
      "0e?"     // ld c,xx
      "41"      // ld b,c
      "16?"     // ld d,xx
      "d9"      // exx
      "21??"    // ld hl,xxxx
      "11??"    // ld de,xxxx
      "01??"    // ld bc,xxxx
      "edb0"    // ldir
      "f9"      // ld sp,hl
      "e1"      // pop hl
      "0e?"     // ld c,xx
      "edb8"    // lddr
      "e1"      // pop hl
      "d1"      // pop de
      "c1"      // pop bc
      "31??"    // ld sp,xxxx
      "c3??"    // jp xxxx
      ,
      0xe5
    }
  };

  const std::size_t DEPACKER_SIZE = 0xe5;

#ifdef USE_PRAGMA_PACK
#pragma pack(push,1)
#endif
  PACK_PRE struct MSPHeader
  {
    //+0
    char Signature[4]; //'M','s','P','k'
    //+4
    uint16_t LastSrcRestBytes;
    //+6
    uint16_t LastSrcPacked;
    //+8
    uint16_t LastDstPacked;
    //+a
    uint16_t SizeOfPacked;
    //+c
    uint16_t DstAddress;
    //+e
    uint8_t BitStream[2];
  } PACK_POST;
#ifdef USE_PRAGMA_PACK
#pragma pack(pop)
#endif

  BOOST_STATIC_ASSERT(sizeof(MSPHeader) == 0x10);

  //msk bitstream decoder (equal to hrust1)
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

  uint_t GetPackedSize(const MSPHeader& header)
  {
    const uint_t lastBytesCount = fromLE(header.LastSrcRestBytes) - fromLE(header.LastSrcPacked);
    return sizeof(MSPHeader) + fromLE(header.SizeOfPacked) + lastBytesCount - 4;
  }

  uint_t GetUnpackedSize(const MSPHeader& header)
  {
    return fromLE(header.LastDstPacked) - fromLE(header.DstAddress) + 1;
  }

  bool CheckMSP(const MSPHeader* header, std::size_t limit)
  {
    BOOST_STATIC_ASSERT(sizeof(header->Signature) == sizeof(MSP_SIGNATURE));
    if (limit < sizeof(*header) ||
        0 != std::memcmp(header->Signature, MSP_SIGNATURE, sizeof(header->Signature)))
    {
      return false;
    }
    if (fromLE(header->LastSrcRestBytes) <= fromLE(header->LastSrcPacked) ||
        fromLE(header->LastDstPacked) <= fromLE(header->DstAddress))
    {
      return false;
    }
    const uint_t packed = GetPackedSize(*header);
    const uint_t unpacked = GetUnpackedSize(*header);
    return fromLE(header->LastSrcRestBytes) > fromLE(header->LastSrcPacked) &&
      packed <= limit &&
      unpacked >= fromLE(header->SizeOfPacked);
  }

  uint_t GetOffset(Bitstream& stream)
  {
    if (stream.GetBit())
    {
      return stream.GetByte() + 1;
    }
    const uint_t code1 = stream.GetBits(3);
    if (code1 < 2)
    {
      return 256 * (code1 + 1) + stream.GetByte() + 1;
    }
    const uint_t code2 = 2 * code1 + stream.GetBit();
    if (code2 < 8)
    {
      return 256 * (code2 - 1) + stream.GetByte() + 1;
    }
    const uint_t code3 = 2 * code2 + stream.GetBit();
    if (code3 < 0x17)
    {
      return 256 * (code3 - 9) + stream.GetByte() + 1;
    }
    const uint_t code4 = (2 * code3 + stream.GetBit()) & 0x1f;
    if (code4 < 0x1f)
    {
      return 256 * code4 + stream.GetByte() + 1;
    }
    const uint_t hidisp = stream.GetByte();
    const uint_t lodisp = stream.GetByte();
    return 256 * hidisp + lodisp + 1;
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

  bool DecodeMSP(const MSPHeader& header, Dump& res)
  {
    const unsigned lastBytesCount = fromLE(header.LastSrcRestBytes) - fromLE(header.LastSrcPacked);
    const uint_t packedSize = fromLE(header.SizeOfPacked);
    const unsigned unpackedSize = GetUnpackedSize(header);
    Dump dst;
    dst.reserve(unpackedSize);

    Bitstream stream(header.BitStream, packedSize);
    while (!stream.Eof())
    {
      //%0 - put byte
      if (!stream.GetBit())
      {
        dst.push_back(stream.GetByte());
        continue;
      }
      uint_t len = 0;
      for (uint_t bits = 3; bits == 0x3 && len != 0xf;)
      {
        bits = stream.GetBits(2);
        len += bits;
      }
      len += 2;
      if (2 == len)
      {
        const unsigned disp = stream.GetByte() + 1;
        if (!CopyFromBack(disp, dst, 2))
        {
          return false;
        }
        continue;
      }
      if (5 == len)
      {
        const unsigned data = stream.GetByte();
        if (0xff == data)
        {
          break;
        }
        if (0xfe == data)
        {
          len = stream.GetByte();
          len += 256 * stream.GetByte();
        }
        else
        {
          len = data + 17;
        }
      }
      else if (len > 5)
      {
        --len;
      }
      const unsigned disp = GetOffset(stream);
      if (!CopyFromBack(disp, dst, len))
      {
        return false;
      }
    }
    if (dst.size() + lastBytesCount != unpackedSize)
    {
      return false;
    }
    const uint8_t* const lastBytes = header.BitStream + packedSize - 2;
    std::copy(lastBytes, lastBytes + lastBytesCount, std::back_inserter(dst));
    res.swap(dst);
    return true;
  }

  class MSPData
  {
  public:
    MSPData(const uint8_t* data, std::size_t size)
      : Header(0)
    {
      const MSPHeader* const header = safe_ptr_cast<const MSPHeader*>(data);
      if (CheckMSP(header, size))
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
      return DecodeMSP(*Header, res);
    }
  private:
    const MSPHeader* Header;
  };

  class MSPPlugin : public ArchivePlugin
                      , private ArchiveDetector
  {
  public:
    virtual String Id() const
    {
      return MSP_PLUGIN_ID;
    }

    virtual String Description() const
    {
      return Text::MSP_PLUGIN_INFO;
    }

    virtual String Version() const
    {
      return MSP_PLUGIN_VERSION;
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
      const MSPData mspData(data, size);
      return mspData.IsValid();
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

      const MSPData mspData(data + region.Offset, limit - region.Offset);
      Dump res;
      if (mspData.Decode(res))
      {
        region.Size = mspData.PackedSize();
        return IO::CreateDataContainer(res);
      }
      return IO::DataContainer::Ptr();
    }
  };
}

namespace ZXTune
{
  void RegisterMSPConvertor(PluginsEnumerator& enumerator)
  {
    const ArchivePlugin::Ptr plugin(new MSPPlugin());
    enumerator.RegisterPlugin(plugin);
  }
}
