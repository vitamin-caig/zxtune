/**
 *
 * @file
 *
 * @brief  Parameters::ZXTune::Sound and tested
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include <sound/backends_parameters.h>
#include <sound/gain.h>
#include <sound/mixer_parameters.h>
#include <sound/sound_parameters.h>

namespace
{
  static_assert(Sound::Gain::CHANNELS == 2, "Incompatible sound channels count");

  using Parameters::ZXTune::Sound::Mixer::PREFIX;
  using Parameters::operator""_id;

  struct MixerValue
  {
    const Parameters::Identifier Name = ""_id;
    const Parameters::IntType DefVal = 0;
  };

  const MixerValue MIXERS[4][4][2] = {
      // 1-channel
      {
          {{PREFIX + "1.0_0"_id, 100}, {PREFIX + "1.0_1"_id, 100}},
      },
      // 2-channel
      {
          {{PREFIX + "2.0_0"_id, 100}, {PREFIX + "2.0_1"_id, 0}},
          {{PREFIX + "2.1_0"_id, 0}, {PREFIX + "2.1_1"_id, 100}},
      },
      // 3-channel
      {
          {{PREFIX + "3.0_0"_id, 100}, {PREFIX + "3.0_1"_id, 0}},
          {{PREFIX + "3.1_0"_id, 60}, {PREFIX + "3.1_1"_id, 60}},
          {{PREFIX + "3.2_0"_id, 0}, {PREFIX + "3.2_1"_id, 100}},
      },
      // 4-channel
      {
          {{PREFIX + "4.0_0"_id, 100}, {PREFIX + "4.0_1"_id, 0}},
          {{PREFIX + "4.1_0"_id, 100}, {PREFIX + "4.1_1"_id, 0}},
          {{PREFIX + "4.2_0"_id, 0}, {PREFIX + "4.2_1"_id, 100}},
          {{PREFIX + "4.3_0"_id, 0}, {PREFIX + "4.3_1"_id, 100}},
      },
  };
}  // namespace

namespace Parameters::ZXTune::Sound::Mixer
{
  Identifier LEVEL(uint_t totalChannels, uint_t inChannel, uint_t outChannel)
  {
    return MIXERS[totalChannels - 1][inChannel][outChannel].Name;
  }

  //! @brief Function to get defaul percent-based level
  IntType LEVEL_DEFAULT(uint_t totalChannels, uint_t inChannel, uint_t outChannel)
  {
    return MIXERS[totalChannels - 1][inChannel][outChannel].DefVal;
  }
}  // namespace Parameters::ZXTune::Sound::Mixer
