#ifndef __MIXER_H_DEFINED__
#define __MIXER_H_DEFINED__

#include <sound.h>

namespace ZXTune
{
  namespace Sound
  {
    /*
      Simple mixer with fixed-point calculations
    */
    template<std::size_t InChannels, std::size_t OutChannels, class C, typename C Precision>
    class Mixer : public Receiver
    {
    public:
      //assume that coeffs for each output channels stored sequently
      Mixer(const C (&matrix)[InChannels][OutChannels], Receiver* delegate)
        : Matrix(matrix), Delegate(delegate)
      {
      }

      virtual void ApplySample(const Sample* input, std::size_t channels)
      {
        assert(channels == InChannels || !"Invalid input channels mixer specified");
        const Sample* mix(&Matrix[0][0]);
        for (Sample* out = &Result[0]; out != &Result[OutChannels]; ++out)
        {
          C res(0);
          for (Sample* in = input; in != input + InChannels; ++in, ++mix)
          {
            res += *in * *mix;
          }
          *out = static_cast<Sample>(res / (Precision * InChannels));
        }
        return Delegate->ApplySample(Result, OutChannels);
      }
    private:
      const C Matrix[InChannels][OutChannels];
      Receiver* const Delegate;
      Sample Result[OutChannels];
    };

    //instantiator with argument type deduction
    template<std::size_t InChannels, std::size_t OutChannels, class C = uint32_t, typename C Precision = 100>
    inline Receiver::Ptr CreateMixer(const C (&matrix)[InChannels][OutChannels], Receiver* delegate)
    {
      return Receiver::Ptr(new Mixer<InChannels, OutChannels, C, Precision>(matrix, delegate));
    }
  }
}

#endif //__MIXER_H_DEFINED__
