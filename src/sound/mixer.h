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

#include "sound/receiver.h"

namespace Sound
{
  template<unsigned Channels>
  class FixedChannelsMixer
  {
  public:
    using InDataType = typename MultichannelSample<Channels>::Type;
    using Ptr = std::shared_ptr<const FixedChannelsMixer<Channels>>;
    virtual ~FixedChannelsMixer() = default;

    virtual Sample ApplyData(const InDataType& in) const = 0;
  };

  using OneChannelMixer = FixedChannelsMixer<1>;
  using TwoChannelsMixer = FixedChannelsMixer<2>;
  using ThreeChannelsMixer = FixedChannelsMixer<3>;
  using FourChannelsMixer = FixedChannelsMixer<4>;
}  // namespace Sound
