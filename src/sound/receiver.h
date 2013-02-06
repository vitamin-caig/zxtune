/**
*
* @file      sound/receiver.h
* @brief     Defenition of sound receiver interface
* @version   $Id$
* @author    (C) Vitamin/CAIG/2001
*
**/

#pragma once
#ifndef __SOUND_RECEIVER_H_DEFINED__
#define __SOUND_RECEIVER_H_DEFINED__

//common includes
#include <data_streaming.h>
//library includes
#include <sound/sound_types.h>

namespace ZXTune
{
  namespace Sound
  {
    //! @brief Simple sound stream endpoint receiver
    typedef DataReceiver<OutputSample> Receiver;
    //! @brief Simple sound stream source
    typedef DataTransmitter<OutputSample> Transmitter;
    //! @brief Simle sound stream converter
    typedef DataTransceiver<OutputSample> Converter;
    //! @brief Channel count-specific receivers
    template<unsigned Channels>
    class FixedChannelsReceiver : public DataReceiver<typename FixedChannelsSample<Channels>::Type> {};

    typedef FixedChannelsReceiver<1> OneChannelReceiver;
    typedef FixedChannelsReceiver<2> TwoChannelsReceiver;
    typedef FixedChannelsReceiver<3> ThreeChannelsReceiver;
    typedef FixedChannelsReceiver<4> FourChannelsReceiver;
  }
}

#endif //__SOUND_RECEIVER_H_DEFINED__
