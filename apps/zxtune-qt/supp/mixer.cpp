/*
Abstract:
  Interactive mixer implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  This file is a part of zxtune-qt application based on zxtune library
*/

//local includes
#include "app_parameters.h"
#include "mixer.h"
#include "playback_supp.h"
//common includes
#include <contract.h>
#include <format.h>
//boost includes
#include <boost/bind.hpp>
#include <boost/make_shared.hpp>

namespace
{
  class MixerMatrix : public ZXTune::Sound::MatrixMixer::Matrix
  {
  public:
    MixerMatrix(Parameters::Accessor::Ptr params, uint_t size, const ZXTune::Sound::MultiGain* defVal)
      : ZXTune::Sound::MatrixMixer::Matrix(defVal, defVal + size)
      , Params(params)
      , Names(size * ZXTune::Sound::OUTPUT_CHANNELS)
    {
      for (uint_t inChan = 0; inChan != size; ++inChan)
      {
        for (uint_t outChan = 0; outChan != ZXTune::Sound::OUTPUT_CHANNELS; ++outChan)
        {
          Names[inChan * ZXTune::Sound::OUTPUT_CHANNELS + outChan] = GetMixerChannelParameterName(size, inChan, outChan);
        }
      }
    }

    void Update()
    {
      std::transform(Names.begin(), Names.end(), begin()->begin(), begin()->begin(), boost::bind(&MixerMatrix::ReadValue, this, _1, _2));
    }
  private:
    ZXTune::Sound::Gain ReadValue(const Parameters::NameType& name, ZXTune::Sound::Gain def) const
    {
      Parameters::IntType val = 0;
      if (Params->FindValue(name, val))
      {
        return ZXTune::Sound::Gain(val) / 100;
      }
      return def;
    }
  private:
    const Parameters::Accessor::Ptr Params;
    std::vector<Parameters::NameType> Names;
  };

  const ZXTune::Sound::MultiGain MIXER1[] =
  {
    { {1.0, 1.0} }
  };

  const ZXTune::Sound::MultiGain MIXER3[] =
  {
    { {1.0, 0.0} },
    { {0.5, 0.5} },
    { {0.0, 1.0} }
  };
  const ZXTune::Sound::MultiGain MIXER4[] =
  {
    { {1.0, 0.0} },
    { {0.7, 0.3} },
    { {0.3, 0.7} },
    { {0.0, 1.0} }
  };

  MixerMatrix CreateMatrix(Parameters::Accessor::Ptr params, uint_t channels)
  {
    switch (channels)
    {
    case 1:
      return MixerMatrix(params, channels, MIXER1);
    case 3:
      return MixerMatrix(params, channels, MIXER3);
    case 4:
      return MixerMatrix(params, channels, MIXER4);
    default:
      Require(!"Invalid channels count");
    }
  }

  class MixerFeedback : public Mixer
  {
  public:
    MixerFeedback(QObject& parent, const MixerMatrix& mtx, ZXTune::Sound::MatrixMixer::Ptr mixer)
      : Mixer(parent)
      , Matrix(mtx)
      , Delegate(mixer)
    {
    }

    virtual void Update()
    {
      Matrix.Update();
      Delegate->SetMatrix(Matrix);
    }
  private:
    MixerMatrix Matrix;
    const ZXTune::Sound::MatrixMixer::Ptr Delegate;
  };

  class MixerWithFeedback : public ZXTune::Sound::Mixer
  {
  public:
    MixerWithFeedback(ZXTune::Sound::MatrixMixer::Ptr mixer, Mixer* feedback)
      : Delegate(mixer)
      , Feedback(feedback)
    {
      Feedback->Update();
    }

    virtual ~MixerWithFeedback()
    {
      Feedback->deleteLater();
    }

    virtual void ApplyData(const std::vector<ZXTune::Sound::Sample>& data)
    {
      Delegate->ApplyData(data);
    }

    virtual void Flush()
    {
      Delegate->Flush();
    }

    virtual void SetTarget(ZXTune::Sound::Receiver::Ptr target)
    {
      Delegate->SetTarget(target);
    }
  private:
    const ZXTune::Sound::MatrixMixer::Ptr Delegate;
    Mixer* const Feedback;
  };
}

namespace Parameters
{
  namespace ZXTuneQT
  {
    namespace Mixers
    {
      const NameType PREFIX = ZXTuneQT::PREFIX + "mixers";
    }
  }
}

Mixer::Mixer(QObject& parent) : QObject(&parent)
{
}

Parameters::NameType GetMixerChannelParameterName(uint_t totalChannels, uint_t inChannel, uint_t outChannel)
{
  const std::string subName = (boost::format("%1%.%2%_%3%") % totalChannels % inChannel % outChannel).str();
  return Parameters::ZXTuneQT::Mixers::PREFIX + subName;
}

int GetMixerChannelDefaultValue(uint_t totalChannels, uint_t inChannel, uint_t outChannel)
{
  switch (totalChannels)
  {
  case 3:
    return int(MIXER3[inChannel][outChannel] * 100 + 0.5);
  case 4:
    return int(MIXER4[inChannel][outChannel] * 100 + 0.5);
  default:
    return 0;
  }
}

ZXTune::Sound::Mixer::Ptr CreateMixer(Parameters::Accessor::Ptr params, uint_t channels)
{
  const ZXTune::Sound::MatrixMixer::Ptr result = ZXTune::Sound::CreateMatrixMixer(channels);
  MixerMatrix matrix = CreateMatrix(params, channels);
  matrix.Update();
  result->SetMatrix(matrix);
  return result;
}

ZXTune::Sound::Mixer::Ptr CreateMixer(PlaybackSupport& supp, Parameters::Accessor::Ptr params, uint_t channels)
{
  const ZXTune::Sound::MatrixMixer::Ptr mixer = ZXTune::Sound::CreateMatrixMixer(channels);
  const MixerMatrix matrix = CreateMatrix(params, channels);
  Mixer* const feedback(new MixerFeedback(supp, matrix, mixer));
  Require(feedback->connect(&supp, SIGNAL(OnUpdateState()), SLOT(Update())));
  return boost::make_shared<MixerWithFeedback>(mixer, feedback);
}
