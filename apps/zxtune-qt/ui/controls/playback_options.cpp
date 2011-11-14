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
#include <core/plugin_attrs.h>
#include <sound/sound_parameters.h>

namespace
{
  class PlaybackOptionsImpl : public PlaybackOptions
                            , private Ui::PlaybackOptions
  {
  public:
    PlaybackOptionsImpl(QWidget& parent, PlaybackSupport& supp, Parameters::Container::Ptr params)
      : ::PlaybackOptions(parent)
      , Params(params)
    {
      //setup self
      setupUi(this);
      AYMOptions->setVisible(false);
      DACOptions->setVisible(false);

      //common
      Parameters::BooleanValue::Bind(*isLooped, *Params, Parameters::ZXTune::Sound::LOOPED, false);
      //AYM
      Parameters::BooleanValue::Bind(*isInterpolated, *Params, Parameters::ZXTune::Core::AYM::INTERPOLATION, false);
      Parameters::BooleanValue::Bind(*isYM, *Params, Parameters::ZXTune::Core::AYM::TYPE, false);
      Parameters::IntegerValue::Bind(*aymLayout, *Params, Parameters::ZXTune::Core::AYM::LAYOUT, 0);
      //DAC
      Parameters::BooleanValue::Bind(*isInterpolated, *Params, Parameters::ZXTune::Core::DAC::INTERPOLATION, false);

      this->connect(&supp, SIGNAL(OnStartModule(ZXTune::Sound::Backend::Ptr, Playlist::Item::Data::Ptr)),
        SLOT(InitState(ZXTune::Sound::Backend::Ptr, Playlist::Item::Data::Ptr)));
      this->connect(&supp, SIGNAL(OnStopModule()), SLOT(CloseState()));
    }

    virtual void InitState(ZXTune::Sound::Backend::Ptr /*player*/, Playlist::Item::Data::Ptr item)
    {
      const ZXTune::Plugin::Ptr plugin = item->GetModule()->GetPlugin();

      const bool isAYM = 0 != (plugin->Capabilities() & ZXTune::CAP_DEV_AYM_MASK);
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
    const Parameters::Container::Ptr Params;
  };
}

PlaybackOptions::PlaybackOptions(QWidget& parent) : QWidget(&parent)
{
}

PlaybackOptions* PlaybackOptions::Create(QWidget& parent, PlaybackSupport& supp, Parameters::Container::Ptr params)
{
  return new PlaybackOptionsImpl(parent, supp, params);
}
