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
#include <core/module_attrs.h>
#include <devices/aym.h>
//std includes
#include <cstring>
//boost includes
#include <boost/make_shared.hpp>
//text includes
#include <formats/text/chiptune.h>

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

  typedef std::vector<Devices::AYM::DataChunk> ChunksArray;

  class ChunksSet : public Formats::Chiptune::PSG::ChunksSet
  {
  public:
    explicit ChunksSet(std::auto_ptr<ChunksArray> data)
      : Data(data)
    {
    }

    virtual std::size_t Count() const
    {
      return Data->size();
    }

    virtual const Devices::AYM::DataChunk& Get(std::size_t frameNum) const
    {
      return Data->at(frameNum);
    }
  private:
    const std::auto_ptr<ChunksArray> Data;  
  };

  class Builder : public Formats::Chiptune::PSG::Builder
  {
  public:
    virtual void AddChunks(std::size_t count)
    {
      if (!Allocate(count))
      {
        Append(count);
      }
    }

    virtual void SetRegister(uint_t reg, uint_t val)
    {
      if (Data.get() && !Data->empty())
      {
        Devices::AYM::DataChunk& chunk = Data->back();
        chunk.Data[reg] = val;
        chunk.Mask |= uint_t(1) << reg;
      }
    }

    virtual Formats::Chiptune::PSG::ChunksSet::Ptr Result() const
    {
      return Formats::Chiptune::PSG::ChunksSet::Ptr(new ChunksSet(Data));
    }
  private:
    bool Allocate(std::size_t count)
    {
      if (!Data.get())
      {
        Data.reset(new ChunksArray(count));
        return true;
      }
      return false;
    }

    void Append(std::size_t count)
    {
      std::fill_n(std::back_inserter(*Data), count, Devices::AYM::DataChunk());
    }
  private:
    mutable std::auto_ptr<ChunksArray> Data;
  };

  class StubBuilder : public Formats::Chiptune::PSG::Builder
  {
  public:
    virtual void AddChunks(std::size_t /*count*/)
    {
    }

    virtual void SetRegister(uint_t /*reg*/, uint_t /*val*/)
    {
    }

    virtual Formats::Chiptune::PSG::ChunksSet::Ptr Result() const
    {
      return Formats::Chiptune::PSG::ChunksSet::Ptr();
    }
  };

  class Properties : public Parameters::Accessor
  {
  public:
    explicit Properties(Binary::Container::Ptr data)
      : Data(data)
    {
    }

    virtual bool FindIntValue(const Parameters::NameType& name, Parameters::IntType& val) const
    {
      if (name == ZXTune::Module::ATTR_CRC ||
          name == ZXTune::Module::ATTR_FIXEDCRC)
      {
        val = Crc32(static_cast<const uint8_t*>(Data->Data()), Data->Size());
        return true;
      }
      else if (name == ZXTune::Module::ATTR_SIZE)
      {
        val = Data->Size();
        return true;
      }
      return false;
    }

    virtual bool FindStringValue(const Parameters::NameType& /*name*/, Parameters::StringType& /*val*/) const
    {
      return false;
    }

    virtual bool FindDataValue(const Parameters::NameType& /*name*/, Parameters::DataType& /*val*/) const
    {
      return false;
    }

    virtual void Process(Parameters::Visitor& visitor) const
    {
      const uint32_t crc = Crc32(static_cast<const uint8_t*>(Data->Data()), Data->Size());
      visitor.SetIntValue(ZXTune::Module::ATTR_CRC, crc);
      visitor.SetIntValue(ZXTune::Module::ATTR_FIXEDCRC, crc);
      visitor.SetIntValue(ZXTune::Module::ATTR_SIZE, Data->Size());
    }
  private:
    const Binary::Container::Ptr Data;
  };

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

    virtual Parameters::Accessor::Ptr GetProperties() const
    {
      return boost::make_shared<Properties>(Delegate);
    }
  private:
    const Binary::Container::Ptr Delegate;
  };

  bool Check(const Binary::Container& rawData)
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
      return ::PSG::Check(rawData);
    }

    virtual Formats::Chiptune::Container::Ptr Decode(const Binary::Container& rawData) const
    {
      Formats::Chiptune::PSG::Builder& stub = Formats::Chiptune::PSG::GetStubBuilder();
      return Formats::Chiptune::PSG::Parse(rawData, stub);
    }
  private:
    const Binary::Format::Ptr Format;
  };
}

namespace Formats
{
  namespace Chiptune
  {
    namespace PSG
    {
      Formats::Chiptune::Container::Ptr Parse(const Binary::Container& rawData, Builder& target)
      {
        if (!::PSG::Check(rawData))
        {
          return Formats::Chiptune::Container::Ptr();
        }

        const Binary::TypedContainer& data(rawData);
        const ::PSG::Header& header = *data.GetField< ::PSG::Header>(0);
        //workaround for some emulators
        const std::size_t offset = (header.Version == ::PSG::INT_BEGIN) ? offsetof(::PSG::Header, Version) : sizeof(header);
        std::size_t size = rawData.Size() - offset;
        const uint8_t* bdata = data.GetField<uint8_t>(offset);
        //detect as much chunks as possible, in despite of real format issues
        while (size)
        {
          const uint_t reg = *bdata;
          ++bdata;
          --size;
          if (::PSG::INT_BEGIN == reg)
          {
            target.AddChunks(1);
          }
          else if (::PSG::INT_SKIP == reg)
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
          else if (::PSG::MUS_END == reg)
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
        return boost::make_shared< ::PSG::Container>(containerData);
      }

      Builder& GetStubBuilder()
      {
        static ::PSG::StubBuilder stub;
        return stub;
      }

      Builder::Ptr CreateBuilder()
      {
        return boost::make_shared< ::PSG::Builder>();
      }
    }

    Decoder::Ptr CreatePSGDecoder()
    {
      return boost::make_shared< ::PSG::Decoder>();
    }
  }
}
