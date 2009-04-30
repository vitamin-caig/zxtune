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
      Tracking() : Position(), Pattern(), Note(), Frame(), Speed(), Channels()
      {
      }
      std::size_t Position;
      std::size_t Pattern;
      std::size_t Note;
      std::size_t Frame;
      std::size_t Speed;
      std::size_t Channels;
    };

    /// Common module information
    struct Information
    {
      Tracking Statistic;
      std::size_t Loop;
      StringMap Properties;
      uint32_t Capabilities;
    };
  }
}

#endif //__MODULE_H_DEFINED__
