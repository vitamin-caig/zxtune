/**
 *
 * @file
 *
 * @brief  Low-pass filter
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include "math/fixedpoint.h"
#include "math/numeric.h"
#include "sound/sample.h"

#include <cmath>

namespace Sound
{
  class LPFilter
  {
  public:
    LPFilter() = default;

    void SetParameters(uint64_t sampleFreq, uint64_t cutOffFreq)
    {
      /*
        A0 * Y[i] = B0 * X[i] + B1 * X[i-1] + B2 * X[i-2] - A1 * Y[i-1] - A2 * Y[i-2]

        For LPF filter:
          d = cutoffFrq / sampleRate
          w0 = 2 * PI * d
          alpha = sin(w0) / 2;

          A0 = 1 + alpha
          A1 = -2 * cos(w0) (always < 0)
          A2 = (1 - alpha)

          B0 = B2 = (1 - cos(w0))/2
          B1 = 2B0

          Y[i] = A(X[i] + 2X[i-1] + X[i-2]) + BY[i-1] - CY[i-2]

          SIN = sin(w0)
          COS = cos(w0)

          A = B0/A0
          B = A1/A0
          C = A2/A0

          http://we.easyelectronics.ru/Theory/chestno-prostoy-cifrovoy-filtr.html
          http://howtodoit.com.ua/tsifrovoy-bih-filtr-iir-filter/
          http://www.ece.uah.edu/~jovanov/CPE621/notes/msp430_filter.pdf
      */
      const float w0 = 3.14159265358f * 2.0f * cutOffFreq / sampleFreq;
      const float q = 1.0f;
      // specify gain to avoid overload
      const float gain = 1.0f;  // 0.98f;

      const float sinus = std::sin(w0);
      const float cosine = std::cos(w0);
      const float alpha = sinus / (2.0f * q);

      const float a0 = (1.0f + alpha) / gain;
      const float a1 = -2.0f * cosine;
      const float a2 = 1.0f - alpha;
      const float b0 = (1.0f - cosine) / 2.0f;

      const float a = b0 / a0;
      const float b = -a1 / a0;
      const float c = a2 / a0;

      // 1 bit- floor rounding
      // 2 bits- scale for A (4)
      DCShift = Math::Log2(static_cast<uint_t>(1.0f / a)) - 3;

      A = a * float(1 << DCShift);
      B = b;
      C = c;

      In1 = In2 = Sample();
      Out1 = Out2 = Sample();
    }

    void Feed(const Sample in)
    {
      const int_t inLeftSum = int_t(in.Left()) + 2 * In1.Left() + In2.Left();
      Coeff sumLeft = A * inLeftSum >> DCShift;
      sumLeft += B * Out1.Left();
      sumLeft -= C * Out2.Left();

      const int_t inRightSum = int_t(in.Right()) + 2 * In1.Right() + In2.Right();
      Coeff sumRight = A * inRightSum >> DCShift;
      sumRight += B * Out1.Right();
      sumRight -= C * Out2.Right();

      Out2 = Out1;
      Out1 = Sample(sumLeft.Integer(), sumRight.Integer());
      In2 = In1;
      In1 = in;
    }

    Sample Get() const
    {
      return Out1;
    }

  private:
    using Coeff = Math::FixedPoint<int_t, 16384>;
    Coeff A, B, C;
    uint_t DCShift = 0;
    Sample In1, In2;
    Sample Out1, Out2;
  };
}  // namespace Sound
