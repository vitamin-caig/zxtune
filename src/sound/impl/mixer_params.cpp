/*
Abstract:
  Mixer on params implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//library includes
#include <parameters/accessor.h>
#include <sound/matrix_mixer.h>
#include <sound/mixer_parameters.h>
//boost includes
#include <boost/make_shared.hpp>

namespace Sound
{
  typedef boost::array<Gain::Type, Sample::CHANNELS> MultiConfigValue;
  const uint_t CONFIG_VALUE_PRECISION = 100;

  void GetMatrixRow(const Parameters::Accessor& params, uint_t channels, uint_t inChan, MultiConfigValue& out)
  {
    for (uint_t outChan = 0; outChan != out.size(); ++outChan)
    {
      const Parameters::NameType name = Parameters::ZXTune::Sound::Mixer::LEVEL(channels, inChan, outChan);
      Parameters::IntType val = Parameters::ZXTune::Sound::Mixer::LEVEL_DEFAULT(channels, inChan, outChan);
      params.FindValue(name, val);
      out[outChan] = Gain::Type(static_cast<uint_t>(val), CONFIG_VALUE_PRECISION);
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
      : Params(params)
      , Mixer(mixer)
      , LastVersion(~params->Version())
    {
    }

    virtual uint_t Version() const
    {
      const uint_t newVers = Params->Version();
      if (newVers != LastVersion)
      {
        FillMixer(*Params, *Mixer);
        LastVersion = newVers;
      }
      return newVers;
    }

    virtual bool FindValue(const Parameters::NameType& name, Parameters::IntType& val) const
    {
      return Params->FindValue(name, val);
    }

    virtual bool FindValue(const Parameters::NameType& name, Parameters::StringType& val) const
    {
      return Params->FindValue(name, val);
    }

    virtual bool FindValue(const Parameters::NameType& name, Parameters::DataType& val) const
    {
      return Params->FindValue(name, val);
    }

    virtual void Process(Parameters::Visitor& visitor) const
    {
      return Params->Process(visitor);
    }
  private:
    const Parameters::Accessor::Ptr Params;
    const typename MixerType::Ptr Mixer;
    mutable uint_t LastVersion;
  };

  template<unsigned Channels>
  Parameters::Accessor::Ptr CreateMixerNotificationParametersInternal(Parameters::Accessor::Ptr params, typename FixedChannelsMatrixMixer<Channels>::Ptr mixer)
  {
    return boost::make_shared<MixerNotificationParameters<FixedChannelsMatrixMixer<Channels> > >(params, mixer);
  }
}

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

  Parameters::Accessor::Ptr CreateMixerNotificationParameters(Parameters::Accessor::Ptr delegate, ThreeChannelsMatrixMixer::Ptr mixer)
  {
    return CreateMixerNotificationParametersInternal<3>(delegate, mixer);
  }

  Parameters::Accessor::Ptr CreateMixerNotificationParameters(Parameters::Accessor::Ptr delegate, FourChannelsMatrixMixer::Ptr mixer)
  {
    return CreateMixerNotificationParametersInternal<4>(delegate, mixer);
  }
}
