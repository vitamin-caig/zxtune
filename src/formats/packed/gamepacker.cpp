/*
Abstract:
  GamePacker support

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
  (C) Based on Pusher sources by Himik/ZxZ
*/

//local includes
#include "container.h"
#include "pack_utils.h"
//common includes
#include <byteorder.h>
#include <tools.h>
//library includes
#include <formats/packed.h>
//std includes
#include <cstring>
//boost includes
#include <boost/make_shared.hpp>

namespace GamePacker
{
  const std::size_t MAX_DECODED_SIZE = 0xc000;

  const std::string DEPACKER_PATTERN =
    "21??"   // ld hl,xxxx depacker body src
    "11??"   // ld de,xxxx depacker body dst
    "d5"     // push de
    "01??"   // ld bc,xxxx depacker body size
    "edb0"   // ldir
    "21??"   // ld hl,xxxx PackedSource
    "11??"   // ld de,xxxx PackedTarget
    "01??"   // ld bc,xxxx PackedSize
    "ed?"    // ldir/lddr (really only lddr is valid in such structure)
    "13"     // inc de
    "eb"     // ex de,hl
    "11??"   // ld de,depack target
    "c9"     // ret
    //+29 (0x1d) DepackerBody starts here
    "7c"     // ld a,h
    "b5"     // or l
  ;

#ifdef USE_PRAGMA_PACK
#pragma pack(push,1)
#endif
  PACK_PRE struct RawHeader
  {
    //+0
    char Padding1;
    //+1
    uint16_t DepackerBodyAddr;
    //+3
    char Padding2[5];
    //+8
    uint16_t DepackerBodySize;
    //+a
    char Padding3[3];
    //+d
    uint16_t PackedSource;
    //+f
    char Padding4;
    //+10
    uint16_t PackedTarget;
    //+12
    char Padding5;
    //+13
    uint16_t PackedSize;
    //+15
    char Padding6;
    //+16
    uint8_t PackedDataCopyDirection;
    //+17
  } PACK_POST;
#ifdef USE_PRAGMA_PACK
#pragma pack(pop)
#endif

  BOOST_STATIC_ASSERT(sizeof(RawHeader) == 0x17);

  class Container
  {
  public:
    Container(const void* data, std::size_t size)
      : Data(static_cast<const uint8_t*>(data))
      , Size(size)
    {
    }

    bool Check() const
    {
      if (Size <= sizeof(RawHeader))
      {
        return false;
      }
      const RawHeader& header = GetHeader();
      /*
      if (fromLE(header.PackedSource) != fromLE(header.DepackerBodyAddr) + fromLE(header.DepackerBodySize))
      {
        return false;
      }
      */
      const DataMovementChecker checker(fromLE(header.PackedSource), fromLE(header.PackedTarget), fromLE(header.PackedSize), header.PackedDataCopyDirection);
      if (!checker.IsValid())
      {
        return false;
      }
      if (checker.LastOfMovedData() != 0xffff)
      {
        return false;
      }
      return GetUsedSize() <= Size;
    }

    uint_t GetUsedSize() const
    {
      const RawHeader& header = GetHeader();
      return fromLE(header.DepackerBodySize) + 0x1d + fromLE(header.PackedSize);
    }

    const uint8_t* GetPackedData() const
    {
      const RawHeader& header = GetHeader();
      return Data + fromLE(header.DepackerBodySize) + 0x1d;
    }

    const RawHeader& GetHeader() const
    {
      assert(Size >= sizeof(RawHeader));
      return *safe_ptr_cast<const RawHeader*>(Data);
    }
  private:
    const uint8_t* const Data;
    const std::size_t Size;
  };

  class DataDecoder
  {
  public:
    explicit DataDecoder(const Container& container)
      : IsValid(container.Check())
      , Header(container.GetHeader())
      , Stream(container.GetPackedData(), fromLE(Header.PackedSize))
      , Result(new Dump())
      , Decoded(*Result)
    {
      if (IsValid && !Stream.Eof())
      {
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
      const uint_t packedSize = fromLE(Header.PackedSize);

      Decoded.reserve(packedSize * 2);

      while (!Stream.Eof() && Decoded.size() < MAX_DECODED_SIZE)
      {
        const uint8_t byt = Stream.GetByte();
        if (0 == (byt & 128))
        {
          const uint_t offset = 256 * (byt & 0x0f) + Stream.GetByte();
          const uint_t count = (byt >> 4) + 3;
          if (!CopyFromBack(offset, Decoded, count))
          {
            return false;
          }
        }
        else if (0 == (byt & 64))
        {
          uint_t count = byt & 63;
          for (; count && !Stream.Eof(); --count)
          {
            Decoded.push_back(Stream.GetByte());
          }
          if (count)
          {
            return false;
          }
        }
        else
        {
          const uint_t data = Stream.GetByte();
          const uint_t len = (byt & 63) + 3;
          std::fill_n(std::back_inserter(Decoded), len, data);
        }
      }
      return true;
    }
  private:
    bool IsValid;
    const RawHeader& Header;
    ByteStream Stream;
    std::auto_ptr<Dump> Result;
    Dump& Decoded;
  };
}


namespace Formats
{
  namespace Packed
  {
    class GamePackerDecoder : public Decoder
    {
    public:
      GamePackerDecoder()
        : Depacker(Binary::Format::Create(GamePacker::DEPACKER_PATTERN))
      {
      }

      virtual Binary::Format::Ptr GetFormat() const
      {
        return Depacker;
      }

      virtual bool Check(const Binary::Container& rawData) const
      {
        const void* const data = rawData.Data();
        const std::size_t availSize = rawData.Size();
        const GamePacker::Container container(data, availSize);
        return container.Check() && Depacker->Match(data, availSize);
      }

      virtual Container::Ptr Decode(const Binary::Container& rawData) const
      {
        const void* const data = rawData.Data();
        const std::size_t availSize = rawData.Size();
        const GamePacker::Container container(data, availSize);
        if (!container.Check() || !Depacker->Match(data, availSize))
        {
          return Container::Ptr();
        }
        GamePacker::DataDecoder decoder(container);
        return CreatePackedContainer(decoder.GetResult(), container.GetUsedSize());
      }
    private:
      const Binary::Format::Ptr Depacker;
    };

    Decoder::Ptr CreateGamePackerDecoder()
    {
      return boost::make_shared<GamePackerDecoder>();
    }
  }
}
