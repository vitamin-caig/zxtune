/**
 *
 * @file
 *
 * @brief Playlist entity interface
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

// local includes
#include "capabilities.h"
// common includes
#include <error.h>
#include <iterator.h>
// library includes
#include <binary/data.h>
#include <module/holder.h>
#include <parameters/container.h>
#include <time/duration.h>

namespace Playlist
{
  namespace Item
  {
    class Data
    {
    public:
      typedef std::shared_ptr<const Data> Ptr;

      virtual ~Data() = default;

      // common
      //  eager objects
      virtual Module::Holder::Ptr GetModule() const = 0;
      virtual Binary::Data::Ptr GetModuleData() const = 0;
      //  lightweight objects
      virtual Parameters::Accessor::Ptr GetModuleProperties() const = 0;
      virtual Parameters::Container::Ptr GetAdjustedParameters() const = 0;
      virtual Capabilities GetCapabilities() const = 0;
      // playlist-related
      virtual Error GetState() const = 0;
      virtual String GetFullPath() const = 0;
      virtual String GetFilePath() const = 0;
      virtual String GetType() const = 0;
      virtual String GetDisplayName() const = 0;
      virtual Time::Milliseconds GetDuration() const = 0;
      virtual String GetAuthor() const = 0;
      virtual String GetTitle() const = 0;
      virtual String GetComment() const = 0;
      virtual uint32_t GetChecksum() const = 0;
      virtual uint32_t GetCoreChecksum() const = 0;
      virtual std::size_t GetSize() const = 0;
    };

    typedef ObjectIterator<Data::Ptr> Collection;
  }  // namespace Item
}  // namespace Playlist
