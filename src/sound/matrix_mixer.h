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
#include <sound/gain.h>
#include <sound/mixer.h>

namespace Sound
{
  template<unsigned Channels>
  class FixedChannelsMatrixMixer : public FixedChannelsMixer<Channels>
  {
  public:
    typedef std::shared_ptr<FixedChannelsMatrixMixer> Ptr;
    typedef boost::array< ::Sound::Gain, Channels> Matrix;

    virtual void SetMatrix(const Matrix& data) = 0;

    static Ptr Create();
  };

  typedef FixedChannelsMatrixMixer<1> OneChannelMatrixMixer;
  typedef FixedChannelsMatrixMixer<2> TwoChannelsMatrixMixer;
  typedef FixedChannelsMatrixMixer<3> ThreeChannelsMatrixMixer;
  typedef FixedChannelsMatrixMixer<4> FourChannelsMatrixMixer;
}
