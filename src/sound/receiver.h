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
//std includes
#include <vector>

namespace ZXTune
{
  namespace Sound
  {
    //! @brief Simple sound stream endpoint receiver
    typedef DataReceiver<MultiSample> Receiver;
    //! @brief Simple sound stream source
    typedef DataTransmitter<MultiSample> Transmitter;
    //! @brief Simle sound stream converter
    typedef DataTransceiver<MultiSample> Converter;
    //! @brief Multichannel stream receiver
    typedef DataReceiver<std::vector<Sample> > MultichannelReceiver;
  }
}

#endif //__SOUND_RECEIVER_H_DEFINED__
