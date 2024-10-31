/**
 *
 * @file
 *
 * @brief  Defenition of mixing-related functionality
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include <sound/gain.h>
#include <sound/mixer.h>

#include <array>

namespace Sound
{
  template<unsigned Channels>
  class FixedChannelsMatrixMixer : public FixedChannelsMixer<Channels>
  {
  public:
    using Ptr = std::shared_ptr<FixedChannelsMatrixMixer>;
    using Matrix = std::array< ::Sound::Gain, Channels>;

    virtual void SetMatrix(const Matrix& data) = 0;

    static Ptr Create();
  };

  using OneChannelMatrixMixer = FixedChannelsMatrixMixer<1>;
  using TwoChannelsMatrixMixer = FixedChannelsMatrixMixer<2>;
  using ThreeChannelsMatrixMixer = FixedChannelsMatrixMixer<3>;
  using FourChannelsMatrixMixer = FixedChannelsMatrixMixer<4>;
}  // namespace Sound
