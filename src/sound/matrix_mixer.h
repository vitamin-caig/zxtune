/**
*
* @file      sound/mixer.h
* @brief     Defenition of mixing-related functionality
* @version   $Id$
* @author    (C) Vitamin/CAIG/2001
*
**/

#pragma once
#ifndef SOUND_MATRIC_MIXER_H_DEFINED
#define SOUND_MATRIC_MIXER_H_DEFINED

//library includes
#include <sound/mixer.h>

namespace ZXTune
{
  namespace Sound
  {
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

#endif //SOUND_MATRIC_MIXER_H_DEFINED
