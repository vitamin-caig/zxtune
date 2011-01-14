/*
Abstract:
  MsPack convertors support

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
#include <numeric>
//boost includes
#include <boost/bind.hpp>
//text includes
#include <core/text/plugins.h>

namespace
{
  using namespace ZXTune;

  const Char MSP_PLUGIN_ID[] = {'M', 'S', 'P', '\0'};
  const String MSP_PLUGIN_VERSION(FromStdString("$Rev$"));

  const char MSP_SIGNATURE[] = {'M', 's', 'P', 'k'};

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
    uint8_t BitStream[0];
  } PACK_POST;
#ifdef USE_PRAGMA_PACK
#pragma pack(pop)
#endif

  BOOST_STATIC_ASSERT(sizeof(MSPHeader) == 0x0e);

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

  unsigned GetPackedSize(const MSPHeader& header)
  {
    const unsigned lastBytesCount = fromLE(header.LastSrcRestBytes) - fromLE(header.LastSrcPacked);
    return sizeof(MSPHeader) + fromLE(header.SizeOfPacked) + lastBytesCount - 2;
  }

  unsigned GetUnpackedSize(const MSPHeader& header)
  {
    return fromLE(header.LastDstPacked) - fromLE(header.DstAddress) + 1;
  }

  unsigned GetOffset(Bitstream& stream)
  {
    if (stream.GetBit())
    {
      return stream.GetByte() + 1;
    }
    const unsigned code1 = stream.GetBits(3);
    if (code1 < 2)
    {
      return 256 * (code1 + 1) + stream.GetByte() + 1;
    }
    const unsigned code2 = 2 * code1 + stream.GetBit();
    if (code2 < 8)
    {
      return 256 * (code2 - 1) + stream.GetByte() + 1;
    }
    const unsigned code3 = 2 * code2 + stream.GetBit();
    if (code3 < 0x17)
    {
      return 256 * (code3 - 9) + stream.GetByte() + 1;
    }
    const unsigned code4 = (2 * code3 + stream.GetBit()) & 0x1f;
    if (code4 < 0x1f)
    {
      return 256 * code4 + stream.GetByte() + 1;
    }
    const unsigned hidisp = stream.GetByte();
    const unsigned lodisp = stream.GetByte();
    return 256 * hidisp + lodisp + 1;
  }

  inline bool CopyFromBack(unsigned offset, Dump& dst, uint_t count)
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

  bool CheckMSP(const MSPHeader& header, std::size_t limit)
  {
    BOOST_STATIC_ASSERT(sizeof(header.Signature) == sizeof(MSP_SIGNATURE));
    if (0 != std::memcmp(header.Signature, MSP_SIGNATURE, sizeof(header.Signature)))
    {
      return false;
    }
    const unsigned packed = GetPackedSize(header);
    const unsigned unpacked = GetUnpackedSize(header);
    return fromLE(header.LastSrcRestBytes) > fromLE(header.LastSrcPacked) &&
      packed <= limit &&
      unpacked >= fromLE(header.SizeOfPacked);
  }

  bool DecodeMSP(const MSPHeader* header, Dump& res)
  {
    const unsigned lastBytesCount = fromLE(header->LastSrcRestBytes) - fromLE(header->LastSrcPacked);
    const uint_t packedSize = fromLE(header->SizeOfPacked);
    const unsigned unpackedSize = GetUnpackedSize(*header);
    Dump dst;

    Bitstream stream(header->BitStream, packedSize);
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
    const uint8_t* const lastBytes = header->BitStream + packedSize - 2;
    std::copy(lastBytes, lastBytes + lastBytesCount, std::back_inserter(dst));
    res.swap(dst);
    return true;
  }

  class MSPPlugin : public ArchivePlugin
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
      const uint8_t* const data = static_cast<const uint8_t*>(inputData.Data());
      const std::size_t limit = inputData.Size();
      if (limit < sizeof(MSPHeader))
      {
        return false;
      }
      const MSPHeader* const header = safe_ptr_cast<const MSPHeader*>(data);
      return CheckMSP(*header, limit);
    }

    virtual IO::DataContainer::Ptr ExtractSubdata(const Parameters::Accessor& /*commonParams*/,
      const MetaContainer& input, ModuleRegion& region) const
    {
      const IO::DataContainer& inputData = *input.Data;
      const uint8_t* const data = static_cast<const uint8_t*>(inputData.Data());
      const MSPHeader* const header = safe_ptr_cast<const MSPHeader*>(data);
      assert(CheckMSP(*header, inputData.Size()));

      Dump res;
      //only check without depacker- depackers are separate
      if (DecodeMSP(header, res))
      {
        region.Offset = 0;
        region.Size = GetPackedSize(*header);
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
