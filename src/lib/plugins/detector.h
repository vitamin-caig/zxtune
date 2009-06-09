#ifndef __DETECTOR_H_DEFINED__
#define __DETECTOR_H_DEFINED__

#include <types.h>

namespace ZXTune
{
  namespace Module
  {
    //Pattern format
    //xx - match byte
    //? - any byte
    bool Detect(const uint8_t* data, std::size_t size, const std::string& pattern);
  }
}

#endif //__DETECTOR_H_DEFINED__
