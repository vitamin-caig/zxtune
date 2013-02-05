/*
Abstract:
  Mixer on params implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//library includes
#include <sound/matrix_mixer.h>
#include <sound/mixer_parameters.h>
//boost includes
#include <boost/make_shared.hpp>

namespace
{
  using namespace ZXTune;

  void GetMatrixRow(const Parameters::Accessor& params, uint_t channels, uint_t inChan, Sound::MultiGain& out)
  {
    for (uint_t outChan = 0; outChan != out.size(); ++outChan)
    {
      const Parameters::NameType name = Parameters::ZXTune::Sound::Mixer::LEVEL(channels, inChan, outChan);
      Parameters::IntType val = Parameters::ZXTune::Sound::Mixer::LEVEL_DEFAULT(channels, inChan, outChan);
      params.FindValue(name, val);
      out[outChan] = Sound::Gain(val) / 100;
    }
  }

  void GetMatrix(const Parameters::Accessor& params, Sound::MultiGain* outBegin, Sound::MultiGain* outEnd)
  {
    const uint_t channels = outEnd - outBegin;
    for (Sound::MultiGain* it = outBegin; it != outEnd; ++it)
    {
      GetMatrixRow(params, channels, it - outBegin, *it);
    }
  }

  template<class MatrixMixer>
  class PollingMixer : public DataTransceiver<typename MatrixMixer::InDataType, Sound::OutputSample>
  {
  public:
    PollingMixer(Parameters::Accessor::Ptr params, typename MatrixMixer::Ptr delegate, const typename MatrixMixer::Matrix& matrix)
      : Params(params)
      , Delegate(delegate)
      , LastMatrix(matrix)
    {
      UpdateMatrix();
    }

    virtual void ApplyData(const typename MatrixMixer::InDataType& inData)
    {
      Delegate->ApplyData(inData);
    }

    virtual void Flush()
    {
      Delegate->Flush();
      UpdateMatrix();
    }
    
    virtual void SetTarget(Sound::Receiver::Ptr rcv)
    {
      Delegate->SetTarget(rcv);
    }
  private:
    void UpdateMatrix()
    {
      const typename MatrixMixer::Matrix oldMatrix = LastMatrix;
      GetMatrix(*Params, &LastMatrix.front(), &LastMatrix.back() + 1);
      if (oldMatrix != LastMatrix)
      {
        Delegate->SetMatrix(LastMatrix);
      }
    }
  private:
    const Parameters::Accessor::Ptr Params;
    const typename MatrixMixer::Ptr Delegate;
    typename MatrixMixer::Matrix LastMatrix;
  };
}

namespace ZXTune
{
  namespace Sound
  {
    template<unsigned Channels>
    typename FixedChannelsMixer<Channels>::Ptr CreateMixer(Parameters::Accessor::Ptr params)
    {
      typedef FixedChannelsMatrixMixer<Channels> MixerType;
      const typename MixerType::Ptr mixer = MixerType::Create();
      typename MixerType::Matrix matrix;
      return boost::make_shared<PollingMixer<MixerType> >(params, mixer, matrix);
    }

    OneChannelMixer::Ptr CreateOneChannelMixer(Parameters::Accessor::Ptr params)
    {
      return CreateMixer<1>(params);
    }

    TwoChannelsMixer::Ptr CreateTwoChannelsMixer(Parameters::Accessor::Ptr params)
    {
      return CreateMixer<2>(params);
    }

    ThreeChannelsMixer::Ptr CreateThreeChannelsMixer(Parameters::Accessor::Ptr params)
    {
      return CreateMixer<3>(params);
    }

    FourChannelsMixer::Ptr CreateFourChannelsMixer(Parameters::Accessor::Ptr params)
    {
      return CreateMixer<4>(params);
    }

    MultichannelMixer::Ptr CreateMultichannelMixer(uint_t channels, Parameters::Accessor::Ptr params)
    {
      const MultichannelMatrixMixer::Ptr mixer = MultichannelMatrixMixer::Create(channels);
      MultichannelMatrixMixer::Matrix matrix(channels);
      return boost::make_shared<PollingMixer<MultichannelMatrixMixer> >(params, mixer, matrix);
    }
  }
}
