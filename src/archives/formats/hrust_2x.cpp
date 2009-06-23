#include "formats_enumerator.h"
#include "hrip.h"

#include "../../lib/io/trdos.h"

#include <types.h>
#include <tools.h>

namespace
{
  using namespace ZXTune;
  using namespace ZXTune::Archive;

  const String::value_type HRUST_21_ID[] = {'H', 'r', 'u', 's', 't', '2', '.', '1', 0};

#ifdef USE_PRAGMA_PACK
#pragma pack(push,1)
#endif
  PACK_PRE struct Hrust2xHeader
  {
    uint8_t LastBytes[6];
    uint8_t FirstByte;
    uint8_t BitStream[1];
  };

  PACK_PRE struct Hrust21Header
  {
    uint8_t ID[3];//'hr2'
    uint8_t Flag;//'1' | 128
    uint16_t DataSize;
    uint16_t PackedSize;
    Hrust2xHeader PackedData;

    //flag bits
    enum
    {
      NO_COMPRESSION = 128
    };
  } PACK_POST;

  const uint8_t HRIP_ID[] = {'H', 'R', 'i'};
  PACK_PRE struct HripHeader
  {
    uint8_t ID[3];//'HRi'
    uint8_t FilesCount;
    uint8_t UsedInLastSector;
    uint16_t ArchiveSectors;
    uint8_t Catalogue;
  } PACK_POST;

  const uint8_t HRIP_BLOCK_ID[] = {'H', 'r', 's', 't', '2'};
  PACK_PRE struct HripBlockHeader
  {
    uint8_t ID[5];//'Hrst2'
    uint8_t Flag;
    uint16_t DataSize;
    uint16_t PackedSize;//without header
    uint8_t AdditionalSize;
    //additional
    uint16_t PackedCRC;
    uint16_t DataCRC;
    IO::TRDosName Filename;
    uint16_t Filesize;
    uint8_t Filesectors;

    //flag bits
    enum
    {
      NO_COMPRESSION = 1,
      LAST_BLOCK = 2,
      DELETED = 32
    };
  } PACK_POST;
#ifdef USE_PRAGMA_PACK
#pragma pack(pop)
#endif

  BOOST_STATIC_ASSERT(sizeof(HripHeader) == 8);
  //BOOST_STATIC_ASSERT(sizeof(HripHeader::ID) == sizeof(HRIP_ID));
  BOOST_STATIC_ASSERT(sizeof(HripBlockHeader) == 29);
  //BOOST_STATIC_ASSERT(sizeof(HripBlockHeader::ID) == sizeof(HRIP_BLOCK_ID));

  class Bitstream
  {
  public:
    Bitstream(const void* data, std::size_t size)
      : Data(static_cast<const uint8_t*>(data)), End(Data + size), Bits(), Mask(0)
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

    uint8_t GetBit()
    {
      if (!(Mask >>= 1))
      {
        Bits = GetByte();
        Mask = 0x80;
      }
      return Bits & Mask ? 1 : 0;
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
    uint8_t Bits;
    uint8_t Mask;
  };

  inline signed GetDist(Bitstream& stream)
  {
    //%1,disp8
    if (stream.GetBit())
    {
      return static_cast<int16_t>(0xff00 + stream.GetByte());
    }
    else
    {
      //%011x,%010xx,%001xxx,%000xxxx,%0000000
      uint16_t res(0xffff);
      for (unsigned bits = 4 - stream.GetBits(2); bits; --bits)
      {
        res = (res << 1) + stream.GetBit() - 1;
      }
      if (0xffe1 == res)
      {
        res = stream.GetByte();
      }
      return static_cast<int16_t>((res << 8) + stream.GetByte());
    }
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

  bool DecodeHrust2x(const Hrust2xHeader* header, unsigned size, Dump& dst)
  {
    //put first byte
    dst.push_back(header->FirstByte);
    //start bitstream
    Bitstream stream(header->BitStream, size);
    while (!stream.Eof())
    {
      //%1,byte
      while (stream.GetBit())
      {
        dst.push_back(stream.GetByte());
      }
      unsigned len(1);
      for (unsigned bits = 3; bits == 0x3 && len != 0x10;)
      {
        bits = stream.GetBits(2), len += bits;
      }
      //%01100..
      if (4 == len)
      {
        //%011001
        if (stream.GetBit())
        {
          unsigned len = stream.GetByte();
          if (!len)
          {
            break;//eof
          }
          else if (len < 16)
          {
            len = len * 256 | stream.GetByte();
          }
          const signed offset = GetDist(stream);
          if (!CopyFromBack(offset, dst, len))
          {
            return false;
          }
        }
        else//%011000xxxx
        {
          for (unsigned len = 2 * (stream.GetBits(4) + 6); len; --len)
          {
            dst.push_back(stream.GetByte());
          }
        }
      }
      else
      {
        if (len > 4)
        {
          --len;
        }
        const signed offset(1 == len ? static_cast<int16_t>(0xfff8 + stream.GetBits(3)) :
        (2 == len ? static_cast<int16_t>(0xff00 + stream.GetByte()) : GetDist(stream)));
        if (!CopyFromBack(offset, dst, len))
        {
          return false;
        }
      }
    }
    std::copy(header->LastBytes, ArrayEnd(header->LastBytes), std::back_inserter(dst));
    return true;
  }

  bool DecodeHrust21(const Hrust21Header* header, Dump& dst)
  {
    if (header->Flag & header->NO_COMPRESSION)
    {
      dst.resize(fromLE(header->DataSize));
      std::memcpy(&dst[0], header->PackedData.LastBytes, dst.size());
      return true;
    }
    dst.reserve(fromLE(header->DataSize));
    return DecodeHrust2x(&header->PackedData, fromLE(header->PackedSize) - 14, dst) && 
      dst.size() == fromLE(header->DataSize);//valid if match
  }

  //append
  bool DecodeHripBlock(const HripBlockHeader* header, Dump& dst)
  {
    const void* const packedData(safe_ptr_cast<const uint8_t*>(&header->PackedCRC) + header->AdditionalSize);
    const std::size_t sizeBefore(dst.size());
    if (header->Flag & header->NO_COMPRESSION)
    {
      dst.resize(sizeBefore + fromLE(header->DataSize));
      std::memcpy(&dst[sizeBefore], packedData, fromLE(header->PackedSize));
      return true;
    }
    dst.reserve(sizeBefore + fromLE(header->DataSize));
    return DecodeHrust2x(static_cast<const Hrust2xHeader*>(packedData), header->PackedSize, dst) && 
      dst.size() == sizeBefore + fromLE(header->DataSize);
  }

  bool Checker(const void* data, std::size_t size)
  {
    const Hrust21Header* const header(static_cast<const Hrust21Header*>(data));
    return (size >= sizeof(*header) && 
      header->ID[0] == 'h' && header->ID[1] == 'r' && header->ID[2] == '2' &&
      fromLE(header->PackedSize) <= fromLE(header->DataSize) &&
      fromLE(header->PackedSize) <= size);
  }

  bool Depacker(const void* data, std::size_t size, Dump& dst)
  {
    assert(Checker(data, size) || !"Attempt to unpack non-checked data");
    return DecodeHrust21(static_cast<const Hrust21Header*>(data), dst);
  }

  AutoFormatRegistrator autoReg(HRUST_21_ID, Checker, Depacker);

  /************ HRIP ******************/
  uint16_t CalcCRC(const uint8_t* data, unsigned size)
  {
    uint16_t result(0);
    while (size--)
    {
      result ^= *data++ << 8;
      for (unsigned i = 0; i < 8; ++i)
      {
        const bool update(0 != (result & 0x8000));
        result <<= 1;
        if (update)
        {
          result ^= 0x1021;
        }
      }
    }
    return result;
  }

  class HripCallback
  {
  public:
    virtual ~HripCallback()
    {
    }

    enum CallbackState
    {
      CONTINUE,
      EXIT,
      ERROR
    };
    virtual CallbackState OnFile(const std::vector<const HripBlockHeader*>&) = 0;
  };

  HripResult ParseHrip(const void* data, std::size_t size, HripCallback& callback)
  {
    if (size < sizeof(HripHeader))
    {
      return INVALID;
    }
    const HripHeader* const hripHeader(static_cast<const HripHeader*>(data));
    if (0 != std::memcmp(hripHeader->ID, HRIP_ID, sizeof(HRIP_ID)) &&
      !(0 == hripHeader->Catalogue || 1 == hripHeader->Catalogue))
    {
      return INVALID;
    }
    const std::size_t archiveSize(256 * (fromLE(hripHeader->ArchiveSectors) - 1) + hripHeader->UsedInLastSector);
    const uint8_t* const ptr(static_cast<const uint8_t*>(data));
    std::size_t offset(sizeof(*hripHeader));
    for (unsigned fileNum = 0; fileNum < hripHeader->FilesCount; ++fileNum)
    {
      std::vector<const HripBlockHeader*> blocks;
      for (;;)//for blocks of file
      {
        const HripBlockHeader* const blockHdr(safe_ptr_cast<const HripBlockHeader*>(ptr + offset));
        const uint8_t* const packedData(safe_ptr_cast<const uint8_t*>(&blockHdr->PackedCRC) + 
          blockHdr->AdditionalSize);
        if (0 != std::memcmp(blockHdr->ID, HRIP_BLOCK_ID, sizeof(HRIP_BLOCK_ID)))
        {
          return CORRUPTED;
        }
        if (packedData + fromLE(blockHdr->PackedSize) > ptr + std::min(size, archiveSize))//out of data
        {
          break;
        }
        if (blockHdr->AdditionalSize > 2 && //here's CRC
          fromLE(blockHdr->PackedCRC) != CalcCRC(packedData, fromLE(blockHdr->PackedSize)))
        {
          return CORRUPTED;
        }
        blocks.push_back(blockHdr);
        offset += fromLE(blockHdr->PackedSize + (packedData - ptr - offset));
        if (blockHdr->Flag & blockHdr->LAST_BLOCK)
        {
          break;
        }
      }
      const HripCallback::CallbackState state(callback.OnFile(blocks));
      if (HripCallback::ERROR == state)
      {
        return CORRUPTED;
      }
      else if (HripCallback::EXIT == state)
      {
        return OK;
      }
    }
    return OK;
  }

  class EnumerateCallback : public HripCallback
  {
  public:
    EnumerateCallback()
    {
    }

    virtual CallbackState OnFile(const std::vector<const HripBlockHeader*>& headers)
    {
      const HripBlockHeader* const header(headers.front());
      Files.push_back(HripFile(IO::GetFileName(header->Filename), header->Filesize));
      return CONTINUE;
    }

    void GetFiles(HripFiles& files)
    {
      files.swap(Files);
    }
  private:
    HripFiles Files;
  };

  class FindExtractCallback : public HripCallback
  {
  public:
    FindExtractCallback(const String& filename, Dump& dst) : Filename(filename), Dst(dst)
    {
    }

    virtual CallbackState OnFile(const std::vector<const HripBlockHeader*>& headers)
    {
      if (Filename == IO::GetFileName(headers.front()->Filename))//found file
      {
        Dst.clear();
        return (headers.end() != std::find_if(headers.begin(), headers.end(), 
          !boost::bind(&DecodeHripBlock, _1, boost::ref(Dst)))) ?
          ERROR : EXIT;
      }
      return CONTINUE;
    }
  private:
    const String Filename;
    Dump& Dst;
  };
}

namespace ZXTune
{
  namespace Archive
  {
    HripResult CheckHrip(const void* data, std::size_t size, HripFiles& files)
    {
      EnumerateCallback callback;
      const HripResult res(ParseHrip(data, size, callback));
      if (OK == res)
      {
        callback.GetFiles(files);
      }
      return res;
    }

    HripResult DepackHrip(const void* data, std::size_t size, const String& file, Dump& dst)
    {
      FindExtractCallback callback(file, dst);
      return ParseHrip(data, size, callback);
    }
  }
}
