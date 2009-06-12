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
    Convertor::Ptr CreateFIRFilter(const std::vector<signed>& coeffs);

    void CalculateFIRCoefficients(uint32_t freq, uint32_t lowCutoff, uint32_t highCutoff, std::vector<signed>& coeffs);
  }
}

#endif //__FILTER_H_DEFINED__
