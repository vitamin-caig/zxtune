/**
*
* @file     formats/chiptune.h
* @brief    Interface for chiptunes data accessors
* @version  $Id$
* @author   (C) Vitamin/CAIG/2001
*
**/

#pragma once
#ifndef __FORMATS_CHIPTUNE_H_DEFINED__
#define __FORMATS_CHIPTUNE_H_DEFINED__

//library includes
#include <binary/container.h>
#include <binary/format.h>
//std includes
#include <memory>

namespace Formats
{
  namespace Chiptune
  {
    //! @brief Chiptune raw data presentation
    class Container : public Binary::Container
    {
    public:
      typedef boost::shared_ptr<const Container> Ptr;

      //! @brief Whole data fingerprint
      //! @return Some integer value at least 32-bit
      virtual uint_t Checksum() const = 0;

      //! @brief Internal structures simple fingerprint
      //! @return Some integer value at least 32-bit
      virtual uint_t FixedChecksum() const = 0;
    };

    //! @brief Decoding functionality provider
    class Decoder
    {
    public:
      typedef boost::shared_ptr<const Decoder> Ptr;
      virtual ~Decoder() {}

      //! @brief Get short decoder description
      virtual String GetDescription() const = 0;

      //! @brief Get approximate format description to search in raw binary data
      //! @invariant Cannot be empty
      virtual Binary::Format::Ptr GetFormat() const = 0;

      //! @brief Fast check for data consistensy
      //! @param rawData Data to be checked
      //! @return false if rawData has defenitely wrong format, else otherwise
      virtual bool Check(const Binary::Container& rawData) const = 0;
      //! @brief Perform raw data decoding
      //! @param rawData Data to be decoded
      //! @return Non-null object if data is successfully recognized and decoded
      //! @invariant Result is always rawData's subcontainer
      virtual Container::Ptr Decode(const Binary::Container& rawData) const = 0;
    };
  }
}

#endif //__FORMATS_CHIPTUNE_H_DEFINED__
