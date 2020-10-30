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

//common includes
#include <data_streaming.h>
//library includes
#include <sound/chunk.h>
#include <sound/multichannel_sample.h>

namespace Sound
{
  //! @brief Simple sound stream endpoint receiver
  typedef DataReceiver<Chunk> Receiver;
  typedef DataConverter<Chunk, Chunk> Converter;
  //! @brief Channel count-specific receivers
  template<unsigned Channels>
  class FixedChannelsReceiver : public DataReceiver<typename MultichannelSample<Channels>::Type> {};

  typedef FixedChannelsReceiver<1> OneChannelReceiver;
  typedef FixedChannelsReceiver<2> TwoChannelsReceiver;
  typedef FixedChannelsReceiver<3> ThreeChannelsReceiver;
  typedef FixedChannelsReceiver<4> FourChannelsReceiver;
}
