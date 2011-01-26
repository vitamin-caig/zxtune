/*
Abstract:
  CodeCrunvher v3 convertors support

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
  (C) Based on XLook sources by HalfElf
*/

//local includes
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
//boost includes
#include <boost/bind.hpp>
//text includes
#include <core/text/plugins.h>

namespace
{
  using namespace ZXTune;

  const Char CC3_PLUGIN_ID[] = {'C', 'C', '3', '\0'};
  const String CC3_PLUGIN_VERSION(FromStdString("$Rev$"));

  const std::size_t DEPACKER_SIZE = 0x99;
  const std::string CC3_DEPACKER_PATTERN(
#if 1
      //decoding loop core pattern
      "+34+"
      "7ecb3f382be60747ed6f234e23e5087e"
#else
#endif
  );

#ifdef USE_PRAGMA_PACK
#pragma pack(push,1)
#endif
  PACK_PRE struct CC3Header
  {
    //+0
    char Padding1[0x17];
    //+17
    uint16_t SizeOfPacked;
    //+19
    char Padding2[0x80];
  } PACK_POST;
#ifdef USE_PRAGMA_PACK
#pragma pack(pop)
#endif

  BOOST_STATIC_ASSERT(sizeof(CC3Header) == DEPACKER_SIZE);

  uint_t GetPackedSize(const CC3Header& header)
  {
    return DEPACKER_SIZE + fromLE(header.SizeOfPacked);
  }

  bool CheckCC3(const CC3Header* header, std::size_t limit)
  {
    if (limit < sizeof(*header))
    {
      return false;
    }
    const uint_t packed = GetPackedSize(*header);
    if (packed > limit)
    {
      return false;
    }
    return DetectFormat(safe_ptr_cast<const uint8_t*>(header), limit, CC3_DEPACKER_PATTERN);
  }

  bool DecodeCC3(const CC3Header& header, Dump& res)
  {
    const uint_t packedSize = fromLE(header.SizeOfPacked);
    const uint8_t* src = safe_ptr_cast<const uint8_t*>(&header) + DEPACKER_SIZE;
    Dump dst;
    dst.reserve(packedSize * 2);//TODO

    ByteStream stream(src, packedSize);
    while (!stream.Eof())
    {
      const uint_t data = stream.GetByte();
      const uint_t loNibble = data & 0x0f;
      const uint_t hiNibble = (data & 0xf0) >> 4;
      if (loNibble == 3)
      {
        //exit
        break;
      }
      //at least one more byte required
      if (stream.Eof())
      {
        return false;
      }
      if (0 == (data & 1))//backref
      {
        const uint_t len = hiNibble + 3;
        const uint_t offset = 128 * loNibble + stream.GetByte();
        if (!CopyFromBack(offset, dst, len))
        {
          return false;
        }
        if (0xf == hiNibble)
        {
          const uint_t len = stream.GetByte();
          if (len && !CopyFromBack(offset, dst, len))
          {
            return false;
          }
        }
      }
      else
      {
        switch (loNibble)
        {
        case 0x09://short RLE
          std::fill_n(std::back_inserter(dst), hiNibble + 3, stream.GetByte());
          break;
        case 0x01://long RLE
          {
            const uint_t len = 256 * hiNibble + stream.GetByte() + 3;
            std::fill_n(std::back_inserter(dst), len, stream.GetByte());
          }
          break;
        case 0x0b://2 bytes
          std::fill_n(std::back_inserter(dst), 2, hiNibble - 1);
          break;
        case 0x05://short copy
          std::generate_n(std::back_inserter(dst), hiNibble + 1, boost::bind(&ByteStream::GetByte, &stream));
          break;
        case 0x0d://long copy
          {
            const uint_t len = 256 * hiNibble + stream.GetByte() + 1;
            std::generate_n(std::back_inserter(dst), len, boost::bind(&ByteStream::GetByte, &stream));
          }
          break;
        default://showr backref
          if (!CopyFromBack((data & 0xf8) >> 3, dst, 2))
          {
            return false;
          }
        }
      }
    }
    res.swap(dst);
    return true;
  }

  class CC3Data
  {
  public:
    CC3Data(const uint8_t* data, std::size_t size)
      : Header(0)
    {
      const CC3Header* const header = safe_ptr_cast<const CC3Header*>(data);
      if (CheckCC3(header, size))
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
      return DecodeCC3(*Header, res);
    }
  private:
    const CC3Header* Header;
  };

  class CC3Plugin : public ArchivePlugin
  {
  public:
    virtual String Id() const
    {
      return CC3_PLUGIN_ID;
    }

    virtual String Description() const
    {
      return Text::CC3_PLUGIN_INFO;
    }

    virtual String Version() const
    {
      return CC3_PLUGIN_VERSION;
    }

    virtual uint_t Capabilities() const
    {
      return CAP_STOR_CONTAINER;
    }

    virtual bool Check(const IO::DataContainer& inputData) const
    {
      const uint8_t* const data = static_cast<const uint8_t*>(inputData.Data());
      const std::size_t size = inputData.Size();
      const CC3Data ccData(data, size);
      return ccData.IsValid();
    }

    virtual IO::DataContainer::Ptr ExtractSubdata(const Parameters::Accessor& /*parameters*/,
      const MetaContainer& input, ModuleRegion& region) const
    {
      const IO::DataContainer& inputData = *input.Data;
      const uint8_t* const data = static_cast<const uint8_t*>(inputData.Data());
      const std::size_t size = inputData.Size();
      const CC3Data ccData(data, size);
      assert(ccData.IsValid());
      Dump res;
      if (ccData.Decode(res))
      {
        region.Size = ccData.PackedSize();
        return IO::CreateDataContainer(res);
      }
      return IO::DataContainer::Ptr();
    }
  };
}

namespace ZXTune
{
  void RegisterCC3Convertor(PluginsEnumerator& enumerator)
  {
    const ArchivePlugin::Ptr plugin(new CC3Plugin());
    enumerator.RegisterPlugin(plugin);
  }
}
