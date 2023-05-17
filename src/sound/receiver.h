/**
 *
 * @file
 *
 * @brief  Defenition of sound receiver interface
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

// common includes
#include <data_streaming.h>
// library includes
#include <sound/chunk.h>
#include <sound/multichannel_sample.h>

namespace Sound
{
  //! @brief Simple sound stream endpoint receiver
  using Receiver = DataReceiver<Chunk>;
  using Converter = DataConverter<Chunk, Chunk>;
  //! @brief Channel count-specific receivers
  template<unsigned Channels>
  class FixedChannelsReceiver : public DataReceiver<typename MultichannelSample<Channels>::Type>
  {};

  using OneChannelReceiver = FixedChannelsReceiver<1>;
  using TwoChannelsReceiver = FixedChannelsReceiver<2>;
  using ThreeChannelsReceiver = FixedChannelsReceiver<3>;
  using FourChannelsReceiver = FixedChannelsReceiver<4>;
}  // namespace Sound
