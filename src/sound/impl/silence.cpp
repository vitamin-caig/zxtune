//common includes
#include <contract.h>
#include <make_ptr.h>
//library includes
#include <sound/silence.h>
#include <sound/sound_parameters.h>

namespace Sound
{
  class SilenceDetector : public Receiver
  {
  public:
    SilenceDetector(uint_t limit, Ptr delegate)
      : Delegate(delegate)
      , Limit(limit)
      , Counter()
    {
    }
    
    void ApplyData(Chunk in) override
    {
      Require(Counter < Limit);
      const std::size_t silent = std::count(in.begin(), in.end(), Sample{});
      if (silent == in.size())
      {
        Counter += silent;
      }
      else
      {
        Counter = 0;
      }
      Delegate->ApplyData(std::move(in));
    }
    
    void Flush() override
    {
      Delegate->Flush();
    }
  private:
    const Ptr Delegate;
    const uint_t Limit;
    uint_t Counter;
  };

  Receiver::Ptr CreateSilenceDetector(Parameters::Accessor::Ptr params, Receiver::Ptr delegate)
  {
    Parameters::IntType freq = Parameters::ZXTune::Sound::FREQUENCY_DEFAULT;
    params->FindValue(Parameters::ZXTune::Sound::FREQUENCY, freq);
    Parameters::IntType silenceLimit = Parameters::ZXTune::Sound::SILENCE_LIMIT_DEFAULT;
    params->FindValue(Parameters::ZXTune::Sound::SILENCE_LIMIT, silenceLimit);
    if (const auto samples = freq * silenceLimit / Parameters::ZXTune::Sound::SILENCE_LIMIT_PRECISION)
    {
      return MakePtr<SilenceDetector>(samples, delegate);
    }
    else
    {
      return delegate;
    }
  }
}
