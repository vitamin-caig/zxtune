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
#include "formats/chiptune/multitrack/multitrack.h"
//common includes
#include <make_ptr.h>
//library includes
#include <binary/container_base.h>
#include <binary/crc.h>
//std includes
#include <utility>

namespace Formats
{
namespace Chiptune
{
  class MultitrackContainer : public Binary::BaseContainer<Container, Multitrack::Container>
  {
  public:
    explicit MultitrackContainer(Multitrack::Container::Ptr data)
      : BaseContainer(std::move(data))
    {
    }

    uint_t Checksum() const override
    {
      return Binary::Crc32(*Delegate);
    }

    uint_t FixedChecksum() const override
    {
      return Delegate->FixedChecksum();
    }
  };

  class MultitrackDecoder : public Decoder
  {
  public:
    MultitrackDecoder(String description, Formats::Multitrack::Decoder::Ptr delegate)
      : Description(std::move(description))
      , Delegate(std::move(delegate))
    {
    }

    String GetDescription() const override
    {
      return Description;
    }

    Binary::Format::Ptr GetFormat() const override
    {
      return Delegate->GetFormat();
    }
    
    bool Check(Binary::View rawData) const override
    {
      return Delegate->Check(rawData);
    }

    Formats::Chiptune::Container::Ptr Decode(const Binary::Container& rawData) const override
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
