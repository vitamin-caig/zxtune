#ifndef __MODULE_H_DEFINED__
#define __MODULE_H_DEFINED__

#include <module_attrs.h>

namespace ZXTune
{
  namespace Module
  {
    /// Time position descriptor
    struct Time
    {
      std::size_t Minute;
      std::size_t Second;
      std::size_t Frame;
    };

    /// Track position descriptor
    struct Tracking
    {
      std::size_t Position;
      std::size_t Pattern;
      std::size_t Note;
      std::size_t Speed;
      unsigned Channels;
    };

    /// Common module information
    struct Information
    {
      Type Type;
      Time Duration;
      Tracking Statistic;
      std::size_t Loop;
      StringMap Properties;
    };
  }
}

#endif //__MODULE_H_DEFINED__
