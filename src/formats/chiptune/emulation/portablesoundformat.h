/**
 *
 * @file
 *
 * @brief  Portable Sound Format family support
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include "formats/chiptune/builder_meta.h"

#include "formats/chiptune.h"
#include "time/duration.h"

namespace Formats::Chiptune::PortableSoundFormat
{
  class Builder
  {
  public:
    virtual ~Builder() = default;

    virtual MetaBuilder& GetMetaBuilder() = 0;

    virtual void SetVersion(uint_t ver) = 0;

    virtual void SetReservedSection(Binary::Container::Ptr blob) = 0;
    virtual void SetPackedProgramSection(Binary::Container::Ptr blob) = 0;

    virtual void SetYear(String date) = 0;
    virtual void SetGenre(String genre) = 0;
    virtual void SetCopyright(String copyright) = 0;
    virtual void SetDumper(String dumper) = 0;

    virtual void SetLength(Time::Milliseconds duration) = 0;
    virtual void SetFade(Time::Milliseconds duration) = 0;

    virtual void SetVolume(float vol) = 0;

    // Tag name is always in lower case
    virtual void SetTag(String name, String value) = 0;
    //! @note number is 1-based and equal to 1 for _lib and X for _libX
    virtual void SetLibrary(uint_t num, String filename) = 0;
  };

  //! @return subcontainer of data
  Container::Ptr Parse(const Binary::Container& data, Builder& target);
}  // namespace Formats::Chiptune::PortableSoundFormat
