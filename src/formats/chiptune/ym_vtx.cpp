/*
Abstract:
  YM/VTX modules format implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//local includes
#include "container.h"
#include "ym.h"
//common includes
#include <byteorder.h>
#include <contract.h>
#include <crc.h>
#include <logging.h>
//library includes
#include <binary/typed_container.h>
#include <formats/packed/lha_supp.h>
//std includes
#include <cstring>
//boost includes
#include <boost/array.hpp>
#include <boost/make_shared.hpp>
//text includes
#include <formats/text/chiptune.h>

namespace
{
  const std::string THIS_MODULE("Formats::Chiptune::YM");

  class ContainerHelper
  {
  public:
    explicit ContainerHelper(const Binary::Container& rawData)
      : Data(rawData)
      , Start(static_cast<const uint8_t*>(rawData.Data()))
      , Finish(Start + rawData.Size())
      , Offset()
    {
    }

    template<class T>
    const T& Read()
    {
      const uint8_t* const cursor = Start + Offset;
      const std::size_t size = sizeof(T);
      const T* const res = safe_ptr_cast<const T*>(cursor);
      Require(cursor + size < Finish);
      Offset += size;
      return *res;
    }

    String ReadString(std::size_t maxSize)
    {
      const uint8_t* const cursor = Start + Offset;
      const uint8_t* const limit = std::min(cursor + maxSize, Finish);
      const uint8_t* const strEnd = std::find(cursor, limit, 0);
      Require(strEnd != limit);
      const String res(cursor, strEnd);
      Offset = strEnd - Start + 1;
      return res;
    }

    const uint8_t* ReadData(std::size_t size)
    {
      const uint8_t* const cursor = Start + Offset;
      Require(cursor + size < Finish);
      Offset += size;
      return cursor;
    }

    Binary::Container::Ptr ReadRestData()
    {
      const std::size_t limit = Data.Size();
      Require(limit > Offset);
      const std::size_t size = limit - Offset;
      Offset = limit;
      return Data.GetSubcontainer(Offset - size, size);
    }

    std::size_t GetOffset() const
    {
      return Offset;
    }
  private:
    const Binary::Container& Data;
    const uint8_t* const Start;
    const uint8_t* const Finish;
    std::size_t Offset;
  };
}

namespace Formats
{
namespace Chiptune
{
  namespace YM
  {
#ifdef USE_PRAGMA_PACK
#pragma pack(push,1)
#endif
    typedef boost::array<uint8_t, 14> RegistersDump;
    typedef boost::array<uint8_t, 4> IdentifierType;

    namespace Ver2
    {
      const uint8_t ID[] = {'Y', 'M', '2', '!'};

      PACK_PRE struct RawHeader
      {
        IdentifierType Signature;
        RegistersDump Row;
      } PACK_POST;

      bool FastCheck(const void* data, std::size_t size)
      {
        return size >= sizeof(RawHeader) && 0 == std::memcmp(data, ID, sizeof(ID));
      }
    }

    namespace Ver3
    {
      const uint8_t ID[] = {'Y', 'M', '3', '!'};

      PACK_PRE struct RawHeader
      {
        IdentifierType Signature;
        RegistersDump Row;
      } PACK_POST;

      bool FastCheck(const void* data, std::size_t size)
      {
        return size >= sizeof(RawHeader) && 0 == std::memcmp(data, ID, sizeof(ID));
      }
    }

    namespace Ver3b
    {
      const uint8_t ID[] = {'Y', 'M', '3', 'b'};

      PACK_PRE struct RawHeader
      {
        IdentifierType Signature;
        RegistersDump Row;
      } PACK_POST;

      bool FastCheck(const void* data, std::size_t size)
      {
        return size >= sizeof(RawHeader) + sizeof(uint32_t) && 0 == std::memcmp(data, ID, sizeof(ID));
      }
    }

    namespace Ver5
    {
      const uint8_t ID[] = {'Y', 'M', '5', '!', 'L', 'e','O', 'n', 'A', 'r', 'D', '!'};

      PACK_PRE struct RawHeader
      {
        uint8_t Signature[12];
        uint32_t Frames;
        uint32_t Attrs;
        uint16_t SamplesCount;
        uint32_t Clockrate;
        uint16_t IntFreq;
        uint32_t Loop;
        uint16_t ExtraSize;
      } PACK_POST;

      bool FastCheck(const void* data, std::size_t size)
      {
        return size >= sizeof(RawHeader) && 0 == std::memcmp(data, ID, sizeof(ID));
      }
    }
#ifdef USE_PRAGMA_PACK
#pragma pack(pop)
#endif

    class StubBuilder : public Builder
    {
    public:
      virtual void SetVersion(const String& /*version*/) {}
      virtual void SetChipType(bool /*ym*/) {}
      virtual void SetStereoMode(uint_t /*mode*/) {}
      virtual void SetLoop(uint_t /*loop*/) {}
      virtual void SetDigitalSample(uint_t /*idx*/, const Dump& /*data*/) {}
      virtual void SetClockrate(uint64_t /*freq*/) {}
      virtual void SetIntFreq(uint_t /*freq*/) {}
      virtual void SetTitle(const String& /*title*/) {}
      virtual void SetAuthor(const String& /*author*/) {}
      virtual void SetComment(const String& /*comment*/) {}
      virtual void SetYear(uint_t /*year*/) {}
      virtual void SetProgram(const String& /*program*/) {}
      virtual void SetEditor(const String& /*editor*/) {}

      virtual void AddData(const Dump& /*registers*/) {}
    };

    bool FastCheck(const Binary::Container& rawData)
    {
      const void* const data = rawData.Data();
      const std::size_t size = rawData.Size();
      return Ver2::FastCheck(data, size)
          || Ver3::FastCheck(data, size)
          || Ver3b::FastCheck(data, size)
          || Ver5::FastCheck(data, size)
      ;
    }

    void ParseTransponedMatrix(const uint8_t* data, std::size_t size, std::size_t columns, Builder& target)
    {
      const std::size_t lines = size / columns;
      Require(lines != 0);
      for (std::size_t row = 0; row != lines; ++row)
      {
        Dump registers(columns);
        for (std::size_t col = 0, cursor = row; col != columns; ++col, cursor += lines)
        {
          registers[col] = data[cursor];
        }
        target.AddData(registers);
      }
    }

    Formats::Chiptune::Container::Ptr ParseUnpacked(const Binary::Container& rawData, Builder& target)
    {
      const void* const data = rawData.Data();
      const std::size_t size = rawData.Size();
      try
      {
        if (Ver2::FastCheck(data, size)
         || Ver3::FastCheck(data, size)
         || Ver3b::FastCheck(data, size))
        {
          ContainerHelper container(rawData);
          const IdentifierType& type = container.Read<IdentifierType>();
          target.SetVersion(String(type.begin(), type.end()));

          const std::size_t columns = sizeof(RegistersDump);
          const std::size_t lines = (size - sizeof(IdentifierType)) / columns;
          const std::size_t matrixSize = lines * columns;
          const uint8_t* const src = container.ReadData(matrixSize);
          ParseTransponedMatrix(src, matrixSize, columns, target);
          if (Ver3b::FastCheck(data, size))
          {
            const uint_t loop = fromBE(container.Read<uint32_t>());
            target.SetLoop(loop);
          }
          const Binary::Container::Ptr subData = rawData.GetSubcontainer(0, container.GetOffset());
          return CreateCalculatingCrcContainer(subData, sizeof(type), lines * columns);
        }
      }
      catch (const std::exception&)
      {
        Log::Debug(THIS_MODULE, "Failed to parse");
      }
      return Formats::Chiptune::Container::Ptr();
    }
  }

  namespace VTX
  {
    using namespace YM;

    const uint_t CHIP_AY = 0x7961;
    const uint_t CHIP_YM = 0x6d79;
    const uint_t CHIP_AY_OLD = 0x5941;
    const uint_t CHIP_YM_OLD = 0x4d59;

    const uint_t LAYOUT_MASK = 7;
    const uint_t LAYOUT_MIN = 0;
    const uint_t LAYOUT_MAX = 6;

    const uint_t CLOCKRATE_MIN = 100000;//100kHz

    const uint_t INTFREQ_MIN = 25;
    const uint_t INTFREQ_MAX = 100;

    const uint_t UNPACKED_MAX = sizeof(RegistersDump) * INTFREQ_MAX * 30 * 60;//30 min

    const std::size_t MAX_STRING_SIZE = 254;

#ifdef USE_PRAGMA_PACK
#pragma pack(push,1)
#endif
    PACK_PRE struct RawBasicHeader
    {
      uint16_t ChipType;
      uint8_t LayoutMode;
      uint16_t Loop;
      uint32_t Clockrate;
      uint8_t IntFreq;
    } PACK_POST;

    PACK_PRE struct RawNewHeader : RawBasicHeader
    {
      uint16_t Year;
      uint32_t UnpackedSize;
    } PACK_POST;

    PACK_PRE struct RawOldHeader : RawBasicHeader
    {
      uint32_t UnpackedSize;
    } PACK_POST;
#ifdef USE_PRAGMA_PACK
#pragma pack(pop)
#endif

    BOOST_STATIC_ASSERT(sizeof(RawBasicHeader) == 10);
    BOOST_STATIC_ASSERT(sizeof(RawNewHeader) == 16);
    BOOST_STATIC_ASSERT(sizeof(RawOldHeader) == 14);

    template<class HeaderType>
    bool FastCheck(const HeaderType& hdr)
    {
      if (!in_range<uint_t>(hdr.LayoutMode & LAYOUT_MASK, LAYOUT_MIN, LAYOUT_MAX))
      {
        return false;
      }
      if (!in_range<uint_t>(hdr.IntFreq, INTFREQ_MIN, INTFREQ_MAX))
      {
        return false;
      }
      if (fromLE(hdr.Clockrate) < CLOCKRATE_MIN)
      {
        return false;
      }
      if (fromLE(hdr.UnpackedSize) > UNPACKED_MAX)
      {
        return false;
      }
      return true;
    }

    bool FastCheck(const Binary::Container& rawData)
    {
      const Binary::TypedContainer typedData(rawData);
      if (const RawBasicHeader* basic = typedData.GetField<RawBasicHeader>(0))
      {
        const uint16_t type = fromLE(basic->ChipType);
        if (type == CHIP_AY || type == CHIP_YM)
        {
          if (const RawNewHeader* hdr = typedData.GetField<RawNewHeader>(0))
          {
            return FastCheck(*hdr);
          }
        }
        else if (type == CHIP_AY_OLD || type == CHIP_YM_OLD)
        {
          if (const RawOldHeader* hdr = typedData.GetField<RawOldHeader>(0))
          {
            return FastCheck(*hdr);
          }
        }
      }
      return false;
    }

    const std::string FORMAT(
      "('a|'A|'y|'Y)('y|'Y|'m|'M)" //type
      "00-06"          //layout
      "??"             //loop
      "????"           //clockrate
      "01-64"          //intfreq, 1..100Hz
    );

    class Decoder : public Formats::Chiptune::Decoder
    {
    public:
      Decoder()
        : Format(Binary::Format::Create(FORMAT))
      {
      }

      virtual String GetDescription() const
      {
        return Text::VTX_DECODER_DESCRIPTION;
      }

      virtual Binary::Format::Ptr GetFormat() const
      {
        return Format;
      }

      virtual bool Check(const Binary::Container& rawData) const
      {
        return FastCheck(rawData);
      }

      virtual Formats::Chiptune::Container::Ptr Decode(const Binary::Container& rawData) const
      {
        Builder& stub = GetStubBuilder();
        return ParseVTX(rawData, stub);
      }
    private:
      const Binary::Format::Ptr Format;
    };
  }//namespace VTX

  namespace YM
  {
    Formats::Chiptune::Container::Ptr ParseVTX(const Binary::Container& rawData, Builder& target)
    {
      if (!VTX::FastCheck(rawData))
      {
        return Formats::Chiptune::Container::Ptr();
      }
      try
      {
        ContainerHelper data(rawData);
        const VTX::RawBasicHeader& hdr = data.Read<VTX::RawBasicHeader>();
        const uint_t chipType = fromLE(hdr.ChipType);
        const bool ym = chipType == VTX::CHIP_YM || chipType == VTX::CHIP_YM_OLD;
        const bool newVersion = chipType == VTX::CHIP_YM || chipType == VTX::CHIP_AY;
        target.SetChipType(ym);
        target.SetStereoMode(hdr.LayoutMode);
        target.SetLoop(fromLE(hdr.Loop));
        target.SetClockrate(fromLE(hdr.Clockrate));
        target.SetIntFreq(hdr.IntFreq);
        if (newVersion)
        {
          target.SetYear(fromLE(data.Read<uint16_t>()));
        }
        const uint_t unpackedSize = fromLE(data.Read<uint32_t>());
        target.SetTitle(data.ReadString(VTX::MAX_STRING_SIZE));
        target.SetAuthor(data.ReadString(VTX::MAX_STRING_SIZE));
        if (newVersion)
        {
          target.SetProgram(data.ReadString(VTX::MAX_STRING_SIZE));
          target.SetEditor(data.ReadString(VTX::MAX_STRING_SIZE));
          target.SetComment(data.ReadString(VTX::MAX_STRING_SIZE));
        }

        const std::size_t packedOffset = data.GetOffset();
        Log::Debug(THIS_MODULE, "Packed data at %1%", packedOffset);
        const Binary::Container::Ptr packed = data.ReadRestData();
        if (Packed::Container::Ptr unpacked = Packed::Lha::DecodeRawData(*packed, "-lh5-", unpackedSize))
        {
          const std::size_t unpackedSize = unpacked->Size();
          Require(0 == (unpackedSize % sizeof(RegistersDump)));
          ParseTransponedMatrix(static_cast<const uint8_t*>(unpacked->Data()), unpackedSize, sizeof(RegistersDump), target);
          const std::size_t packedSize = unpacked->PackedSize();
          const Binary::Container::Ptr subData = rawData.GetSubcontainer(0, packedOffset + packedSize);
          return CreateCalculatingCrcContainer(subData, packedOffset, packedSize);
        }
        Log::Debug(THIS_MODULE, "Failed to decode ym data");
      }
      catch (const std::exception&)
      {
        Log::Debug(THIS_MODULE, "Failed to parse");
      }
      return Formats::Chiptune::Container::Ptr();
    }

    Builder& GetStubBuilder()
    {
      static StubBuilder stub;
      return stub;
    }
  }

  Decoder::Ptr CreateVTXDecoder()
  {
    return boost::make_shared<VTX::Decoder>();
  }
}//namespace Chiptune
}//namespace Formats
