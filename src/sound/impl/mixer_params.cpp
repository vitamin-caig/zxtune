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

  Sound::MatrixMixer::Matrix GetMatrix(uint_t channels, const Parameters::Accessor& params)
  {
    Sound::MatrixMixer::Matrix res(channels);
    for (uint_t inChan = 0; inChan != channels; ++inChan)
    {
      Sound::MultiGain& out = res[inChan];
      for (uint_t outChan = 0; outChan != out.size(); ++outChan)
      {
        const Parameters::NameType name = Parameters::ZXTune::Sound::Mixer::LEVEL(channels, inChan, outChan);
        Parameters::IntType val = Parameters::ZXTune::Sound::Mixer::LEVEL_DEFAULT(channels, inChan, outChan);
        params.FindValue(name, val);
        out[outChan] = Sound::Gain(val) / 100;
      }
    }
    return res;
  }

  class PollingMixer : public Sound::Mixer
  {
  public:
    PollingMixer(uint_t channels, Parameters::Accessor::Ptr params)
      : Channels(channels)
      , Params(params)
      , Delegate(Sound::CreateMatrixMixer(Channels))
    {
      UpdateMatrix();
    }

    virtual void ApplyData(const std::vector<Sound::Sample>& inData)
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
      const Sound::MatrixMixer::Matrix newMatrix = GetMatrix(Channels, *Params);
      if (newMatrix != LastMatrix)
      {
        LastMatrix = newMatrix;
        Delegate->SetMatrix(LastMatrix);
      }
    }
  private:
    const uint_t Channels;
    const Parameters::Accessor::Ptr Params;
    const Sound::MatrixMixer::Ptr Delegate;
    Sound::MatrixMixer::Matrix LastMatrix;
  };
}

namespace ZXTune
{
  namespace Sound
  {
    Mixer::Ptr CreateMixer(uint_t channels, Parameters::Accessor::Ptr params)
    {
      const Sound::MatrixMixer::Ptr res = Sound::CreateMatrixMixer(channels);
      const Sound::MatrixMixer::Matrix matrix = GetMatrix(channels, *params);
      res->SetMatrix(matrix);
      return res;
    }

    Mixer::Ptr CreatePollingMixer(uint_t channels, Parameters::Accessor::Ptr params)
    {
      return boost::make_shared<PollingMixer>(channels, params);
    }
  }
}
