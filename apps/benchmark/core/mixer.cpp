/*
Abstract:
  Mixer test

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//local includes
#include "mixer.h"
#include "timer.h"
//library includes
#include <sound/matrix_mixer.h>

namespace Benchmark
{
  namespace Mixer
  {
    double Test(uint_t channels, const Time::Milliseconds& duration, uint_t soundFreq)
    {
      const ZXTune::Sound::Mixer::Ptr mixer = ZXTune::Sound::CreateMatrixMixer(channels);

      std::vector<ZXTune::Sound::Sample> input(channels);
      const Timer timer;
      const uint_t totalFrames = uint64_t(duration.Get()) * soundFreq / duration.PER_SECOND;
      for (uint_t frame = 0; frame != totalFrames; ++frame)
      {
        mixer->ApplyData(input);
      }
      mixer->Flush();
      const Time::Nanoseconds elapsed = timer.Elapsed<Time::Nanoseconds>();
      const Time::Nanoseconds emulated(duration);
      return double(emulated.Get()) / elapsed.Get();

    }
  }
}
