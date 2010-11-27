/*
Abstract:
  Render params implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//library includes
#include <sound/render_params.h>
#include <sound/sound_parameters.h>
//boost includes
#include <boost/make_shared.hpp>

namespace
{
  class RenderParametersImpl : public ZXTune::Sound::RenderParameters
  {
  public:
    explicit RenderParametersImpl(Parameters::Accessor::Ptr params)
      : Params(params)
    {
    }

    virtual uint64_t ClockFreq() const
    {
      using namespace Parameters::ZXTune::Sound;
      return FoundProperty(CLOCKRATE, CLOCKRATE_DEFAULT);
    }

    virtual uint_t SoundFreq() const
    {
      using namespace Parameters::ZXTune::Sound;
      return static_cast<uint_t>(FoundProperty(FREQUENCY, FREQUENCY_DEFAULT));
    }

    virtual uint_t FrameDurationMicrosec() const
    {
      using namespace Parameters::ZXTune::Sound;
      return static_cast<uint_t>(FoundProperty(FRAMEDURATION, FRAMEDURATION_DEFAULT));
    }

    virtual ZXTune::Sound::LoopMode Looping() const
    {
      using namespace Parameters::ZXTune::Sound;
      return static_cast<ZXTune::Sound::LoopMode>(FoundProperty(LOOPMODE, LOOPMODE_DEFAULT));
    }

    virtual uint_t ClocksPerFrame() const
    {
      const uint64_t clock = ClockFreq();
      const uint_t frameDuration = FrameDurationMicrosec();
      return static_cast<uint_t>(clock * frameDuration / 1000000); 
    }

    virtual uint_t SamplesPerFrame() const
    {
      const uint_t sound = SoundFreq();
      const uint_t frameDuration = FrameDurationMicrosec();
      return static_cast<uint_t>(sound * frameDuration / 1000000);
    }
  private:
    Parameters::IntType FoundProperty(const Parameters::NameType& name, Parameters::IntType defVal) const
    {
      Parameters::IntType ret = defVal;
      Params->FindIntValue(name, ret);
      return ret;
    }
  private:
    const Parameters::Accessor::Ptr Params;
  };
}

namespace ZXTune
{
  namespace Sound
  {
    RenderParameters::Ptr RenderParameters::Create(Parameters::Accessor::Ptr soundParameters)
    {
      return boost::make_shared<RenderParametersImpl>(soundParameters);
    }
  }
}
