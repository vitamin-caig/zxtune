/**
 *
 * @file
 *
 * @brief  Mixer test implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "mixer.h"

#include "sound/matrix_mixer.h"
#include "sound/mixer.h"
#include "sound/multichannel_sample.h"
#include "time/timer.h"

namespace Benchmark::Mixer
{
  template<unsigned Channels>
  double Test(const Time::Milliseconds& duration, uint_t soundFreq)
  {
    const typename Sound::FixedChannelsMixer<Channels>::Ptr mixer = Sound::FixedChannelsMatrixMixer<Channels>::Create();

    const typename Sound::MultichannelSample<Channels>::Type input{};
    const Time::Timer timer;
    const uint_t totalFrames = uint64_t(duration.Get()) * soundFreq / duration.PER_SECOND;
    for (uint_t frame = 0; frame != totalFrames; ++frame)
    {
      mixer->ApplyData(input);
    }
    const auto elapsed = timer.Elapsed();
    return duration.Divide<double>(elapsed);
  }

  double Test(uint_t channels, const Time::Milliseconds& duration, uint_t soundFreq)
  {
    switch (channels)
    {
    case 1:
      return Test<1>(duration, soundFreq);
    case 2:
      return Test<2>(duration, soundFreq);
    case 3:
      return Test<3>(duration, soundFreq);
    case 4:
      return Test<4>(duration, soundFreq);
    default:
      return 0;
    }
  }
}  // namespace Benchmark::Mixer
