/**
 *
 * @file
 *
 * @brief  Parameters for mixer
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// common includes
#include <make_ptr.h>
// library includes
#include <parameters/accessor.h>
#include <sound/matrix_mixer.h>
#include <sound/mixer_parameters.h>
#include <sound/sound_parameters.h>
// std includes
#include <utility>

namespace Sound
{
  using MultiConfigValue = std::array<Gain::Type, Sample::CHANNELS>;

  void GetMatrixRow(const Parameters::Accessor& params, uint_t channels, uint_t inChan, MultiConfigValue& out)
  {
    for (uint_t outChan = 0; outChan != out.size(); ++outChan)
    {
      using namespace Parameters::ZXTune::Sound;
      const auto name = Mixer::LEVEL(channels, inChan, outChan);
      const auto def = Mixer::LEVEL_DEFAULT(channels, inChan, outChan);
      const auto val = Parameters::GetInteger<int_t>(params, name, def);
      out[outChan] = Gain::Type(val, GAIN_PRECISION);
    }
  }

  template<unsigned Channels>
  void GetMatrix(const Parameters::Accessor& params, typename FixedChannelsMatrixMixer<Channels>::Matrix& res)
  {
    MultiConfigValue row;
    for (uint_t inChan = 0; inChan != res.size(); ++inChan)
    {
      GetMatrixRow(params, Channels, inChan, row);
      res[inChan] = Gain(row[0], row[1]);
    }
  }

  template<unsigned Channels>
  void FillMixerInternal(const Parameters::Accessor& params, FixedChannelsMatrixMixer<Channels>& mixer)
  {
    typename FixedChannelsMatrixMixer<Channels>::Matrix res;
    GetMatrix<Channels>(params, res);
    mixer.SetMatrix(res);
  }

  template<class MixerType>
  class MixerNotificationParameters : public Parameters::Accessor
  {
  public:
    MixerNotificationParameters(Parameters::Accessor::Ptr params, typename MixerType::Ptr mixer)
      : Params(std::move(params))
      , Mixer(std::move(mixer))
      , LastVersion(~Params->Version())
    {}

    uint_t Version() const override
    {
      const uint_t newVers = Params->Version();
      if (newVers != LastVersion)
      {
        FillMixer(*Params, *Mixer);
        LastVersion = newVers;
      }
      return newVers;
    }

    std::optional<Parameters::IntType> FindInteger(Parameters::Identifier name) const override
    {
      return Params->FindInteger(name);
    }

    std::optional<Parameters::StringType> FindString(Parameters::Identifier name) const override
    {
      return Params->FindString(name);
    }

    Binary::Data::Ptr FindData(Parameters::Identifier name) const override
    {
      return Params->FindData(name);
    }

    void Process(Parameters::Visitor& visitor) const override
    {
      return Params->Process(visitor);
    }

  private:
    const Parameters::Accessor::Ptr Params;
    const typename MixerType::Ptr Mixer;
    mutable uint_t LastVersion;
  };

  template<unsigned Channels>
  Parameters::Accessor::Ptr
      CreateMixerNotificationParametersInternal(Parameters::Accessor::Ptr params,
                                                typename FixedChannelsMatrixMixer<Channels>::Ptr mixer)
  {
    return MakePtr<MixerNotificationParameters<FixedChannelsMatrixMixer<Channels> > >(params, mixer);
  }
}  // namespace Sound

namespace Sound
{
  void FillMixer(const Parameters::Accessor& params, ThreeChannelsMatrixMixer& mixer)
  {
    FillMixerInternal<3>(params, mixer);
  }

  void FillMixer(const Parameters::Accessor& params, FourChannelsMatrixMixer& mixer)
  {
    FillMixerInternal<4>(params, mixer);
  }

  Parameters::Accessor::Ptr CreateMixerNotificationParameters(Parameters::Accessor::Ptr delegate,
                                                              ThreeChannelsMatrixMixer::Ptr mixer)
  {
    return CreateMixerNotificationParametersInternal<3>(std::move(delegate), std::move(mixer));
  }

  Parameters::Accessor::Ptr CreateMixerNotificationParameters(Parameters::Accessor::Ptr delegate,
                                                              FourChannelsMatrixMixer::Ptr mixer)
  {
    return CreateMixerNotificationParametersInternal<4>(std::move(delegate), std::move(mixer));
  }
}  // namespace Sound
