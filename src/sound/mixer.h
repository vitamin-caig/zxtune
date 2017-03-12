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

//library includes
#include <sound/receiver.h>

namespace Sound
{
  template<unsigned Channels>
  class FixedChannelsMixer
  {
  public:
    typedef typename MultichannelSample<Channels>::Type InDataType;
    typedef std::shared_ptr<const FixedChannelsMixer<Channels> > Ptr;
    virtual ~FixedChannelsMixer() = default;

    virtual Sample ApplyData(const InDataType& in) const = 0;
  };

  typedef FixedChannelsMixer<1> OneChannelMixer;
  typedef FixedChannelsMixer<2> TwoChannelsMixer;
  typedef FixedChannelsMixer<3> ThreeChannelsMixer;
  typedef FixedChannelsMixer<4> FourChannelsMixer;
}
