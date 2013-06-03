/**
*
* @file      sound/mixer.h
* @brief     Defenition of mixing-related functionality
* @version   $Id$
* @author    (C) Vitamin/CAIG/2001
*
**/

#pragma once
#ifndef SOUND_MIXER_H_DEFINED
#define SOUND_MIXER_H_DEFINED

//library includes
#include <sound/receiver.h>

namespace Sound
{
  template<unsigned Channels>
  class FixedChannelsStreamMixer : public DataTransceiver<typename MultichannelSample<Channels>::Type, Sample> {};

  typedef FixedChannelsStreamMixer<1> OneChannelStreamMixer;
  typedef FixedChannelsStreamMixer<2> TwoChannelsStreamMixer;
  typedef FixedChannelsStreamMixer<3> ThreeChannelsStreamMixer;
  typedef FixedChannelsStreamMixer<4> FourChannelsStreamMixer;

  template<unsigned Channels>
  class FixedChannelsMixer
  {
  public:
    typedef typename MultichannelSample<Channels>::Type InDataType;
    typedef boost::shared_ptr<const FixedChannelsMixer<Channels> > Ptr;
    virtual ~FixedChannelsMixer() {}

    virtual Sample ApplyData(const InDataType& in) const = 0;
  };

  typedef FixedChannelsMixer<1> OneChannelMixer;
  typedef FixedChannelsMixer<2> TwoChannelsMixer;
  typedef FixedChannelsMixer<3> ThreeChannelsMixer;
  typedef FixedChannelsMixer<4> FourChannelsMixer;
}

#endif //SOUND_MIXER_H_DEFINED
