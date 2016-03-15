/**
* 
* @file
*
* @brief  Multitrack chiptunes support implementation
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "multitrack.h"
//common includes
#include <crc.h>
#include <make_ptr.h>

namespace Formats
{
namespace Chiptune
{
  class MultitrackContainer : public Container
  {
  public:
    explicit MultitrackContainer(Formats::Multitrack::Container::Ptr data)
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

  class MultitrackDecoder : public Decoder
  {
  public:
    MultitrackDecoder(const String& description, Formats::Multitrack::Decoder::Ptr delegate)
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

  Container::Ptr CreateMultitrackChiptuneContainer(Formats::Multitrack::Container::Ptr delegate)
  {
    return MakePtr<MultitrackContainer>(delegate);
  }
  
  Decoder::Ptr CreateMultitrackChiptuneDecoder(const String& description, Formats::Multitrack::Decoder::Ptr delegate)
  {
    return MakePtr<MultitrackDecoder>(description, delegate);
  }
}
}
