#include "mixer.h"

namespace
{
  using namespace ZXTune::Sound;

  /*
  Simple mixer with fixed-point calculations
  */
  template<std::size_t InChannels>
  class Mixer : public Receiver
  {
  public:
    //assume that coeffs for each input channels stored sequently
    //because quantity of input channels differs in different types of devices while output is constant
    Mixer(const Sample* matrix, Receiver* delegate)
      : Matrix(matrix), Delegate(delegate)
    {

    }

    virtual void ApplySample(const Sample* input, std::size_t channels)
    {
      assert(channels == InChannels || !"Invalid input channels mixer specified");
      const Sample* mix(Matrix);
      for (const Sample* out = Result; out != Result + OUTPUT_CHANNELS; ++out, ++mix)
      {
        BigSample res(0);
        for (const Sample* in = input; in != input + InChannels; ++in, mix += OUTPUT_CHANNELS)
        {
          res += *in * *mix;
        }
        mix -= OUTPUT_CHANNELS * InChannels;
        *out = static_cast<Sample>(res / (FIXED_POINT_PRECISION * InChannels));
      }
      return Delegate->ApplySample(Result, OUTPUT_CHANNELS);
    }
  private:
    const Sample* const Matrix;
    Receiver* const Delegate;
    Sample Result[OUTPUT_CHANNELS];
  };

  const unsigned CHANNELS_AYM = 3;
  const unsigned CHANNELS_BEEPER = 1;
  const unsigned CHANNELS_SOUNDRIVE = 4;

  class MixerManagerImpl : public MixerManager
  {
  public:
    MixerManagerImpl()
    {
    }

  private:
    Sample MatrixAYMBeeper[OUTPUT_CHANNELS * (CHANNELS_AYM + CHANNELS_BEEPER)];
    Sample MatrixSoundrive[OUTPUT_CHANNELS * CHANNELS_SOUNDRIVE];
  };
}
