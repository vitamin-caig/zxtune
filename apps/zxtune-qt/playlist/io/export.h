/**
* 
* @file
*
* @brief Playlist export interface
*
* @author vitamin.caig@gmail.com
*
**/

#pragma once

//local includes
#include "container.h"

class QString;
namespace Playlist
{
  namespace IO
  {
    class ExportCallback
    {
    public:
      virtual ~ExportCallback() {}

      virtual void Progress(unsigned percents) = 0;
      virtual bool IsCanceled() const = 0;
    };

    enum ExportFlagValues
    {
      SAVE_ATTRIBUTES = 1,
      RELATIVE_PATHS = 2,
      SAVE_CONTENT = 4
    };

    typedef uint_t ExportFlags;

    Error SaveXSPF(Container::Ptr container, const QString& filename, ExportCallback& cb, ExportFlags flags);
  }
}
