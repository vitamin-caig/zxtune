/**
 *
 * @file
 *
 * @brief  Declaration of fast mixer
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include <sound/gain.h>
#include <sound/multichannel_sample.h>

#include <array>

namespace Sound
{
  template<int_t ChannelsCount>
  class MixerCore
  {
  public:
    using MatrixType = std::array<Gain, ChannelsCount>;
    using InType = typename MultichannelSample<ChannelsCount>::Type;

    MixerCore()
    {
      const Coeff val = Coeff(1, ChannelsCount);
      for (uint_t inChan = 0; inChan != ChannelsCount; ++inChan)
      {
        CoeffRow& row = Matrix[inChan];
        for (auto& outChan : row)
        {
          outChan = val;
        }
      }
    }

    Sample Mix(const InType& in) const
    {
      Coeff out[Sample::CHANNELS];
      for (uint_t inChan = 0; inChan != ChannelsCount; ++inChan)
      {
        const int_t val = in[inChan];
        const CoeffRow& row = Matrix[inChan];
        for (uint_t outChan = 0; outChan != row.size(); ++outChan)
        {
          out[outChan] += row[outChan] * val;
        }
      }
      static_assert(Sample::CHANNELS == 2, "Incompatible sound channels count");
      return {out[0].Integer(), out[1].Integer()};
    }

    void SetMatrix(const MatrixType& matrix)
    {
      for (uint_t inChan = 0; inChan != ChannelsCount; ++inChan)
      {
        const Gain& in = matrix[inChan];
        CoeffRow& out = Matrix[inChan];
        out[0] = Coeff(in.Left() / ChannelsCount);
        out[1] = Coeff(in.Right() / ChannelsCount);
      }
    }

  private:
    static const int_t PRECISION = 256;
    using Coeff = Math::FixedPoint<int_t, PRECISION>;
    using CoeffRow = std::array<Coeff, Sample::CHANNELS>;
    using CoeffMatrix = std::array<CoeffRow, ChannelsCount>;
    CoeffMatrix Matrix;
  };
}  // namespace Sound
