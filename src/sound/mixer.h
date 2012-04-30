/**
*
* @file      sound/mixer.h
* @brief     Defenition of mixing-related functionality
* @version   $Id$
* @author    (C) Vitamin/CAIG/2001
*
**/

#pragma once
#ifndef __SOUND_MIXER_H_DEFINED__
#define __SOUND_MIXER_H_DEFINED__

//common includes
#include <error.h>
//library includes
#include <sound/receiver.h>

namespace ZXTune
{
  namespace Sound
  {
    //! @brief Abstract mixer interface
    typedef DataTransceiver<std::vector<Sample>, MultiSample> Mixer;

    //! @brief Matrix-based mixer interface
    class MatrixMixer : public Mixer
    {
    public:
      //! @brief Pointer type
      typedef boost::shared_ptr<MatrixMixer> Ptr;
      //! @brief Matrix type
      typedef std::vector<MultiGain> Matrix;

      //! @brief Setting up the mixing matrix
      //! @param data Mixing matrix
      //! @return Error() in case of success
      virtual void SetMatrix(const Matrix& data) = 0;
    };

    //! @brief Creating mixer instance
    //! @param channels Input channels count
    //! @note For any of the created mixers, SetMatrix parameter size should be equal to channels parameter while creating
    MatrixMixer::Ptr CreateMatrixMixer(uint_t channels);
  }
}

#endif //__SOUND_MIXER_H_DEFINED__
