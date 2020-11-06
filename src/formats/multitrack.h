/**
* 
* @file
*
* @brief  Interfaces for multitrack chiptunes with undivideable tracks (e.g. SID)
*
* @author vitamin.caig@gmail.com
*
**/

#pragma once

//common includes
#include <types.h>
//library includes
#include <binary/container.h>
#include <binary/format.h>
//std includes
#include <memory>

namespace Formats
{
  namespace Multitrack
  {
    class Container : public Binary::Container
    {
    public:
      typedef std::shared_ptr<const Container> Ptr;
      
      //! Same as in Chiptune::Container
      virtual uint_t FixedChecksum() const = 0;
      
      //! @return total tracks count
      virtual uint_t TracksCount() const = 0;

      //! @return 0-based index of first track
      virtual uint_t StartTrackIndex() const = 0;
      
      //! @brief Create copy of self with modified start track 0-based index
      virtual Container::Ptr WithStartTrackIndex(uint_t idx) const = 0;
    };
    
    class Decoder
    {
    public:
      typedef std::shared_ptr<const Decoder> Ptr;
      virtual ~Decoder() = default;
      
      //! @brief Get approximate format description to search in raw binary data
      //! @invariant Cannot be empty
      virtual Binary::Format::Ptr GetFormat() const = 0;

      //! @brief Fast check for data consistensy
      //! @param rawData Data to be checked
      //! @return false if rawData has defenitely wrong format, else otherwise
      virtual bool Check(Binary::View rawData) const = 0;
      
      //! @brief Perform raw data decoding
      //! @param rawData Data to be decoded
      //! @return Non-null object if data is successfully recognized and decoded
      //! @invariant Result is always rawData's subcontainer
      virtual Container::Ptr Decode(const Binary::Container& rawData) const = 0;
    };
  }
}
