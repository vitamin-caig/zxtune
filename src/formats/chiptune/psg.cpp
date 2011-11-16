/*
Abstract:
  PSG modules format implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//local includes
#include "psg.h"
//common includes
#include <crc.h>
#include <tools.h>
//library includes
#include <binary/typed_container.h>
#include <devices/aym.h>
//std includes
#include <cstring>
//boost includes
#include <boost/make_shared.hpp>
//text includes
#include <formats/text/chiptune.h>

namespace Formats
{
namespace Chiptune
{
  namespace PSG
  {
    enum
    {
      MARKER = 0x1a,

      INT_BEGIN = 0xff,
      INT_SKIP = 0xfe,
      MUS_END = 0xfd
    };

    const uint8_t SIGNATURE[] = {'P', 'S', 'G'};

#ifdef USE_PRAGMA_PACK
#pragma pack(push,1)
#endif
    PACK_PRE struct Header
    {
      uint8_t Sign[3];
      uint8_t Marker;
      uint8_t Version;
      uint8_t Interrupt;
      uint8_t Padding[10];
    } PACK_POST;
#ifdef USE_PRAGMA_PACK
#pragma pack(pop)
#endif

    BOOST_STATIC_ASSERT(sizeof(Header) == 16);

    class Container : public Formats::Chiptune::Container
    {
    public:
      explicit Container(Binary::Container::Ptr delegate)
        : Delegate(delegate)
      {
      }

      virtual std::size_t Size() const
      {
        return Delegate->Size();
      }

      virtual const void* Data() const
      {
        return Delegate->Data();
      }

      virtual Binary::Container::Ptr GetSubcontainer(std::size_t offset, std::size_t size) const
      {
        return Delegate->GetSubcontainer(offset, size);
      }

      virtual uint_t FixedChecksum() const
      {
        const Binary::TypedContainer& data(*Delegate);
        const Header& header = *data.GetField<Header>(0);
        const std::size_t dataOffset = (header.Version == INT_BEGIN) ? offsetof(Header, Version) : sizeof(header);
        return Crc32(data.GetField<uint8_t>(dataOffset), Delegate->Size() - dataOffset);
      }
    private:
      const Binary::Container::Ptr Delegate;
    };

    class StubBuilder : public Builder
    {
    public:
      virtual void AddChunks(std::size_t /*count*/) {}
      virtual void SetRegister(uint_t /*reg*/, uint_t /*val*/) {}
    };

    bool FastCheck(const Binary::Container& rawData)
    {
      if (rawData.Size() <= sizeof(Header))
      {
        return false;
      }
      const Header* const header = safe_ptr_cast<const Header*>(rawData.Data());
      return 0 == std::memcmp(header->Sign, SIGNATURE, sizeof(SIGNATURE)) &&
         MARKER == header->Marker;
    }

    const std::string FORMAT(
      "'P'S'G" // uint8_t Sign[3];
      "1a"     // uint8_t Marker;
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
        return Text::PSG_DECODER_DESCRIPTION;
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
        return Parse(rawData, stub);
      }
    private:
      const Binary::Format::Ptr Format;
    };

    Formats::Chiptune::Container::Ptr Parse(const Binary::Container& rawData, Builder& target)
    {
      if (!FastCheck(rawData))
      {
        return Formats::Chiptune::Container::Ptr();
      }

      const Binary::TypedContainer& data(rawData);
      const Header& header = *data.GetField<Header>(0);
      //workaround for some emulators
      const std::size_t offset = (header.Version == INT_BEGIN) ? offsetof(Header, Version) : sizeof(header);
      std::size_t size = rawData.Size() - offset;
      const uint8_t* bdata = data.GetField<uint8_t>(offset);
      //detect as much chunks as possible, in despite of real format issues
      while (size)
      {
        const uint_t reg = *bdata;
        ++bdata;
        --size;
        if (INT_BEGIN == reg)
        {
          target.AddChunks(1);
        }
        else if (INT_SKIP == reg)
        {
          if (size < 1)
          {
            ++size;//put byte back
            break;
          }
          target.AddChunks(4 * *bdata);
          ++bdata;
          --size;
        }
        else if (MUS_END == reg)
        {
          break;
        }
        else if (reg <= 15) //register
        {
          if (size < 1)
          {
            ++size;//put byte back
            break;
          }
          target.SetRegister(reg, *bdata);
          ++bdata;
          --size;
        }
        else
        {
          ++size;//put byte back
          break;
        }
      }
      const Binary::Container::Ptr containerData = rawData.GetSubcontainer(0, rawData.Size() - size);
      return boost::make_shared<Container>(containerData);
    }

    Builder& GetStubBuilder()
    {
      static StubBuilder stub;
      return stub;
    }
  }//namespace PSG

  Decoder::Ptr CreatePSGDecoder()
  {
    return boost::make_shared<PSG::Decoder>();
  }
}//namespace Chiptune
}//namespace Formats
