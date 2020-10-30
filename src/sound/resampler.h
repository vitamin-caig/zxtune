/**
*
* @file
*
* @brief  Defenition of resampling-related functionality
*
* @author vitamin.caig@gmail.com
*
**/

#pragma once

//library includes
#include <sound/receiver.h>

namespace Sound
{
  Sound::Receiver::Ptr CreateResampler(uint_t inFreq, uint_t outFreq, Sound::Receiver::Ptr delegate);

  Converter::Ptr CreateResampler(uint_t inFreq, uint_t outFreq);
}
