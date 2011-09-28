/*
Abstract:
  Hrust2 support

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
  (C) Based on XLook sources by HalfElf
*/

//local includes
#include "container.h"
#include "hrust1_bitstream.h"
#include "pack_utils.h"
//common includes
#include <byteorder.h>
#include <tools.h>
//library includes
#include <formats/packed.h>
//std includes
#include <numeric>
#include <cstring>
//boost includes
#include <boost/make_shared.hpp>
//text includes
#include <formats/text/packed.h>

namespace Hrust2
{
  const std::size_t MAX_DECODED_SIZE = 0xc000;

  const uint8_t SIGNATURE[] = {'h', 'r', '2'};

#ifdef USE_PRAGMA_PACK
#pragma pack(push,1)
#endif
  PACK_PRE struct RawHeader
  {
    uint8_t LastBytes[6];
    uint8_t FirstByte;
    uint8_t BitStream[1];
  } PACK_POST;

  PACK_PRE struct FormatHeader
  {
    uint8_t ID[3];//'hr2'
    uint8_t Flag;//'1' | 128
    uint16_t DataSize;
    uint16_t PackedSize;
    RawHeader Stream;

    //flag bits
    enum
    {
      NO_COMPRESSION = 128
    };
  } PACK_POST;
#ifdef USE_PRAGMA_PACK
#pragma pack(pop)
#endif

  //hrust2x bitstream decoder
  class Bitstream : public ByteStream
  {
  public:
    Bitstream(const uint8_t* data, std::size_t size)
      : ByteStream(data, size)
      , Bits(), Mask(0)
    {
    }

    uint_t GetBit()
    {
      if (!(Mask >>= 1))
      {
        Bits = GetByte();
        Mask = 0x80;
      }
      return Bits & Mask ? 1 : 0;
    }

    uint_t GetBits(uint_t count)
    {
      uint_t result = 0;
      while (count--)
      {
        result = 2 * result | GetBit();
      }
      return result;
    }

    uint_t GetLen()
    {
      uint_t len = 1;
      for (uint_t bits = 3; bits == 0x3 && len != 0x10;)
      {
        bits = GetBits(2);
        len += bits;
      }
      return len;
    }

    int_t GetDist()
    {
      //%1,disp8
      if (GetBit())
      {
        return static_cast<int16_t>(0xff00 + GetByte());
      }
      else
      {
        //%011x,%010xx,%001xxx,%000xxxx,%0000000
        uint_t res = 0xffff;
        for (uint_t bits = 4 - GetBits(2); bits; --bits)
        {
          res = (res << 1) + GetBit() - 1;
        }
        res &= 0xffff;
        if (0xffe1 == res)
        {
          res = GetByte();
        }
        return static_cast<int16_t>((res << 8) + GetByte());
      }
    }
  private:
    uint_t Bits;
    uint_t Mask;
  };

  class Container
  {
  public:
    Container(const void* data, std::size_t size)
      : Data(static_cast<const uint8_t*>(data))
      , Size(size)
    {
    }

    bool FastCheck() const
    {
      if (Size < sizeof(FormatHeader))
      {
        return false;
      }
      const FormatHeader& header = GetHeader();
      if (header.ID[0] != SIGNATURE[0] ||
          header.ID[1] != SIGNATURE[1] ||
          header.ID[2] != SIGNATURE[2])
      {
        return false;
      }
      if (0 != (header.Flag & header.NO_COMPRESSION) &&
          header.PackedSize != header.DataSize)
      {
        return false;
      }
      const std::size_t usedSize = GetUsedSize();
      return in_range(usedSize, sizeof(header), Size);
    }

    uint_t GetUsedSize() const
    {
      const FormatHeader& header = GetHeader();
      return sizeof(header) + fromLE(header.PackedSize) - sizeof(header.Stream);
    }

    const FormatHeader& GetHeader() const
    {
      assert(Size >= sizeof(FormatHeader));
      return *safe_ptr_cast<const FormatHeader*>(Data);
    }
  private:
    const uint8_t* const Data;
    const std::size_t Size;
  };

  class RawDataDecoder
  {
  public:
    RawDataDecoder(const RawHeader& header, std::size_t rawSize)
      : Header(header)
      , Stream(Header.BitStream, rawSize)
      , IsValid(!Stream.Eof())
      , Result(new Dump())
      , Decoded(*Result)
    {
      if (IsValid)
      {
        Decoded.reserve(rawSize * 2);
        IsValid = DecodeData();
      }
    }

    std::auto_ptr<Dump> GetResult()
    {
      return IsValid
        ? Result
        : std::auto_ptr<Dump>();
    }
  private:
    bool DecodeData()
    {
      //put first byte
      Decoded.push_back(Header.FirstByte);

      while (!Stream.Eof() && Decoded.size() < MAX_DECODED_SIZE)
      {
        //%1,byte
        if (Stream.GetBit())
        {
          Decoded.push_back(Stream.GetByte());
          continue;
        }
        uint_t len = Stream.GetLen();
        //%01100..
        if (4 == len)
        {
          //%011001
          if (Stream.GetBit())
          {
            uint_t len = Stream.GetByte();
            if (!len)
            {
              break;//eof
            }
            else if (len < 16)
            {
              len = len * 256 | Stream.GetByte();
            }
            const int_t offset = Stream.GetDist();
            if (!CopyFromBack(-offset, Decoded, len))
            {
              return false;
            }
          }
          else//%011000xxxx
          {
            for (uint_t len = 2 * (Stream.GetBits(4) + 6); len; --len)
            {
              Decoded.push_back(Stream.GetByte());
            }
          }
        }
        else
        {
          if (len > 4)
          {
            --len;
          }
          const int_t offset = 1 == len
            ? static_cast<int16_t>(0xfff8 + Stream.GetBits(3))
            : (2 == len ? static_cast<int16_t>(0xff00 + Stream.GetByte()) : Stream.GetDist());
          if (!CopyFromBack(-offset, Decoded, len))
          {
            return false;
          }
        }
      }
      std::copy(Header.LastBytes, ArrayEnd(Header.LastBytes), std::back_inserter(Decoded));
      return true;
    }
  private:
    const RawHeader& Header;
    Bitstream Stream;
    bool IsValid;
    std::auto_ptr<Dump> Result;
    Dump& Decoded;
  };

  class DataDecoder
  {
  public:
    explicit DataDecoder(const Container& container)
      : IsValid(container.FastCheck())
      , Header(container.GetHeader())
      , Result(new Dump())
    {
      IsValid = IsValid && DecodeData();
    }

    std::auto_ptr<Dump> GetResult()
    {
      return IsValid
        ? Result
        : std::auto_ptr<Dump>();
    }
  private:
    bool DecodeData()
    {
      const uint_t size = fromLE(Header.DataSize);
      if (0 != (Header.Flag & Header.NO_COMPRESSION))
      {
        //just copy
        Result->resize(size);
        std::memcpy(&(*Result)[0], &Header.Stream, size);
        return true;
      }
      RawDataDecoder decoder(Header.Stream, fromLE(Header.PackedSize));
      Result = decoder.GetResult();
      return 0 != Result.get();
    }
  private:
    bool IsValid;
    const FormatHeader& Header;
    std::auto_ptr<Dump> Result;
  };

  class RawFormat : public Binary::Format
  {
  public:
    virtual bool Match(const void* /*data*/, std::size_t size) const
    {
      return size >= sizeof(RawHeader);
    }

    virtual std::size_t Search(const void* /*data*/, std::size_t size) const
    {
      return size >= sizeof(RawHeader) ? 0 : size;
    }
  };

  class Format : public Binary::Format
  {
  public:
    virtual bool Match(const void* data, std::size_t size) const
    {
      if (ArraySize(SIGNATURE) > size)
      {
        return false;
      }
      return std::equal(SIGNATURE, ArrayEnd(SIGNATURE), static_cast<const uint8_t*>(data));
    }

    virtual std::size_t Search(const void* data, std::size_t size) const
    {
      if (ArraySize(SIGNATURE) > size)
      {
        return size;
      }
      const uint8_t* const rawData = static_cast<const uint8_t*>(data);
      return std::search(rawData, rawData + size, SIGNATURE, ArrayEnd(SIGNATURE)) - rawData;
    }
  };
}

namespace Formats
{
  namespace Packed
  {
    class Hrust2RawDecoder : public Decoder
    {
    public:
      virtual String GetDescription() const
      {
        return Text::HRUST2X_DECODER_DESCRIPTION;
      }

      virtual Binary::Format::Ptr GetFormat() const
      {
        return boost::make_shared<Hrust2::RawFormat>();
      }

      virtual bool Check(const Binary::Container& rawData) const
      {
        const std::size_t availSize = rawData.Size();
        return availSize >= sizeof(Hrust2::RawHeader);
      }

      virtual Container::Ptr Decode(const Binary::Container& rawData) const
      {
        const void* const data = rawData.Data();
        const std::size_t availSize = rawData.Size();
        if (availSize < sizeof(Hrust2::RawHeader))
        {
          return Container::Ptr();
        }
        const Hrust2::RawHeader& header = *static_cast<const Hrust2::RawHeader*>(data);
        Hrust2::RawDataDecoder decoder(header, availSize);
        return CreatePackedContainer(decoder.GetResult(), availSize);
      }
    };

    class Hrust21Decoder : public Decoder
    {
    public:
      virtual String GetDescription() const
      {
        return Text::HRUST21_DECODER_DESCRIPTION;
      }

      virtual Binary::Format::Ptr GetFormat() const
      {
        return boost::make_shared<Hrust2::Format>();
      }

      virtual bool Check(const Binary::Container& rawData) const
      {
        const void* const data = rawData.Data();
        const std::size_t availSize = rawData.Size();
        const Hrust2::Container container(data, availSize);
        return container.FastCheck();
      }

      virtual Container::Ptr Decode(const Binary::Container& rawData) const
      {
        const void* const data = rawData.Data();
        const std::size_t availSize = rawData.Size();
        const Hrust2::Container container(data, availSize);
        if (!container.FastCheck())
        {
          return Container::Ptr();
        }
        Hrust2::DataDecoder decoder(container);
        return CreatePackedContainer(decoder.GetResult(), container.GetUsedSize());
      }
    };

    Decoder::Ptr CreateHrust21Decoder()
    {
      return Decoder::Ptr(new Hrust21Decoder());
    }

    Decoder::Ptr CreateHrust2RawDecoder()
    {
      return boost::make_shared<Hrust2RawDecoder>();
    }
  }
}

