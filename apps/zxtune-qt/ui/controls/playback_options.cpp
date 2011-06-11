/*
Abstract:
  Playback options widget implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  This file is a part of zxtune-qt application based on zxtune library
*/

//local includes
#include "playback_options.h"
#include "playback_options.ui.h"
#include "supp/playback_supp.h"
#include "ui/tools/parameters_helpers.h"
//library includes
#include <core/core_parameters.h>
#include <sound/sound_parameters.h>

namespace
{
  class PlaybackOptionsImpl : public PlaybackOptions
                            , private Ui::PlaybackOptions
  {
  public:
    PlaybackOptionsImpl(QWidget& parent, PlaybackSupport& supp, Parameters::Container& params)
      : ::PlaybackOptions(parent)
      , Params(params)
    {
      //setup self
      setupUi(this);
      AYMOptions->setVisible(false);
      DACOptions->setVisible(false);

      //common
      Parameters::BooleanValue::Bind(*actionLoop, Params, Parameters::ZXTune::Sound::LOOPED, false);
      //AYM
      Parameters::BooleanValue::Bind(*actionAYMInterpolate, Params, Parameters::ZXTune::Core::AYM::INTERPOLATION, false);
      Parameters::BooleanValue::Bind(*actionYM, Params, Parameters::ZXTune::Core::AYM::TYPE, false);
      //DAC
      Parameters::BooleanValue::Bind(*actionDACInterpolate, Params, Parameters::ZXTune::Core::DAC::INTERPOLATION, false);

      this->connect(&supp, SIGNAL(OnStartModule(ZXTune::Sound::Backend::Ptr)), SLOT(InitState(ZXTune::Sound::Backend::Ptr)));
      this->connect(&supp, SIGNAL(OnStopModule()), SLOT(CloseState()));
    }

    virtual void InitState(ZXTune::Sound::Backend::Ptr player)
    {
      const ZXTune::Module::Information::Ptr info = player->GetModuleInformation();

      //TODO
      const bool isAYM = info->PhysicalChannels() == 3;
      AYMOptions->setVisible(isAYM);
      DACOptions->setVisible(!isAYM);
      (isAYM ? AYMOptions : DACOptions)->setEnabled(true);
    }

    virtual void CloseState()
    {
      AYMOptions->setEnabled(false);
      DACOptions->setEnabled(false);
    }
  private:
    Parameters::Container& Params;
  };
}

PlaybackOptions::PlaybackOptions(QWidget& parent) : QWidget(&parent)
{
}

PlaybackOptions* PlaybackOptions::Create(QWidget& parent, PlaybackSupport& supp, Parameters::Container& params)
{
  return new PlaybackOptionsImpl(parent, supp, params);
}
