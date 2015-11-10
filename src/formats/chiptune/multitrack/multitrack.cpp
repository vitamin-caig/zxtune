/**
* 
* @file
*
* @brief  Multitrack chiptunes support implementation
*
* @author vitamin.caig@gmail.com
*
**/

//common includes
#include <crc.h>
//local includes
#include "multitrack.h"
//boost includes
#include <boost/make_shared.hpp>

namespace MultitrackChiptunes
{
  class Container : public Formats::Chiptune::Container
  {
  public:
    explicit Container(Formats::Multitrack::Container::Ptr data)
      : Delegate(data)
    {
    }

    //Binary::Container
    virtual const void* Start() const
    {
      return Delegate->Start();
    }

    virtual std::size_t Size() const
    {
      return Delegate->Size();
    }

    virtual Binary::Container::Ptr GetSubcontainer(std::size_t offset, std::size_t size) const
    {
      return Delegate->GetSubcontainer(offset, size);
    }

    //Formats::Chiptune::Container
    virtual uint_t Checksum() const
    {
      return Crc32(static_cast<const uint8_t*>(Delegate->Start()), Delegate->Size());
    }

    virtual uint_t FixedChecksum() const
    {
      return Delegate->FixedChecksum();
    }
  private:
    const Formats::Multitrack::Container::Ptr Delegate;
  };

  class Decoder : public Formats::Chiptune::Decoder
  {
  public:
    Decoder(const String& description, Formats::Multitrack::Decoder::Ptr delegate)
      : Description(description)
      , Delegate(delegate)
    {
    }

    virtual String GetDescription() const
    {
      return Description;
    }

    virtual Binary::Format::Ptr GetFormat() const
    {
      return Delegate->GetFormat();
    }
    
    virtual bool Check(const Binary::Container& rawData) const
    {
      return Delegate->Check(rawData);
    }

    virtual Formats::Chiptune::Container::Ptr Decode(const Binary::Container& rawData) const
    {
      if (const Formats::Multitrack::Container::Ptr data = Delegate->Decode(rawData))
      {
        return Formats::Chiptune::CreateMultitrackChiptuneContainer(data);
      }
      return Formats::Chiptune::Container::Ptr();
    }
  private:
    const String Description;
    const Formats::Multitrack::Decoder::Ptr Delegate;
  };
}

namespace Formats
{
  namespace Chiptune
  {
    Formats::Chiptune::Container::Ptr CreateMultitrackChiptuneContainer(Formats::Multitrack::Container::Ptr delegate)
    {
      return boost::make_shared<MultitrackChiptunes::Container>(delegate);
    }
    
    Formats::Chiptune::Decoder::Ptr CreateMultitrackChiptuneDecoder(const String& description, Formats::Multitrack::Decoder::Ptr delegate)
    {
      return boost::make_shared<MultitrackChiptunes::Decoder>(description, delegate);
    }
  }
}
