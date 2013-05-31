/**
*
* @file      sound/receiver.h
* @brief     Defenition of sound receiver interface
* @version   $Id$
* @author    (C) Vitamin/CAIG/2001
*
**/

#pragma once
#ifndef SOUND_RECEIVER_H_DEFINED
#define SOUND_RECEIVER_H_DEFINED

//common includes
#include <data_streaming.h>
//library includes
#include <sound/multichannel_sample.h>

namespace Sound
{
  //! @brief Simple sound stream endpoint receiver
  typedef DataReceiver<Sample> Receiver;
  //! @brief Simple sound stream source
  typedef DataTransmitter<Sample> Transmitter;
  //! @brief Simle sound stream converter
  typedef DataTransceiver<Sample> Converter;
  //! @brief Channel count-specific receivers
  template<unsigned Channels>
  class FixedChannelsReceiver : public DataReceiver<typename MultichannelSample<Channels>::Type> {};

  typedef FixedChannelsReceiver<1> OneChannelReceiver;
  typedef FixedChannelsReceiver<2> TwoChannelsReceiver;
  typedef FixedChannelsReceiver<3> ThreeChannelsReceiver;
  typedef FixedChannelsReceiver<4> FourChannelsReceiver;
}

#endif //__SOUND_RECEIVER_H_DEFINED__
