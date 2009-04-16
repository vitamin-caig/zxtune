#ifndef __FILTER_H_DEFINED__
#define __FILTER_H_DEFINED__

#include <sound.h>

namespace ZXTune
{
  namespace Sound
  {
    /*
      FIR filter with fixed-point calculations
    */
    Receiver::Ptr CreateFIRFilter(const Sample* coeffs, std::size_t order, Receiver* delegate);
  }
}

#endif //__FILTER_H_DEFINED__
