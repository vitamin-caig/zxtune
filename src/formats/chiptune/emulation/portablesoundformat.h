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

// library includes
#include <formats/chiptune.h>
#include <formats/chiptune/builder_meta.h>
#include <time/duration.h>

namespace Formats
{
  namespace Chiptune
  {
    namespace PortableSoundFormat
    {
      class Builder
      {
      public:
        virtual ~Builder() = default;

        virtual MetaBuilder& GetMetaBuilder() = 0;

        virtual void SetVersion(uint_t ver) = 0;

        virtual void SetReservedSection(Binary::Container::Ptr blob) = 0;
        virtual void SetPackedProgramSection(Binary::Container::Ptr blob) = 0;

        virtual void SetYear(StringView date) = 0;
        virtual void SetGenre(StringView genre) = 0;
        virtual void SetCopyright(StringView copyright) = 0;
        virtual void SetDumper(StringView dumper) = 0;

        virtual void SetLength(Time::Milliseconds duration) = 0;
        virtual void SetFade(Time::Milliseconds duration) = 0;

        virtual void SetVolume(float vol) = 0;

        virtual void SetTag(StringView name, StringView value) = 0;
        //! @note number is 1-based and equal to 1 for _lib and X for _libX
        virtual void SetLibrary(uint_t num, StringView filename) = 0;
      };

      //! @return subcontainer of data
      Container::Ptr Parse(const Binary::Container& data, Builder& target);
    }  // namespace PortableSoundFormat
  }    // namespace Chiptune
}  // namespace Formats
