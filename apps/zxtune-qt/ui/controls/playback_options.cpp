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
#include "playlist/supp/capabilities.h"
#include "supp/playback_supp.h"
#include "ui/tools/parameters_helpers.h"
//common includes
#include <contract.h>
//library includes
#include <core/core_parameters.h>
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
      Parameters::BooleanValue::Bind(*isYM, *Params, Parameters::ZXTune::Core::AYM::TYPE, false);
      Parameters::IntegerValue::Bind(*aymLayout, *Params, Parameters::ZXTune::Core::AYM::LAYOUT, 0);
      //DAC
      Parameters::BooleanValue::Bind(*isDACInterpolated, *Params, Parameters::ZXTune::Core::DAC::INTERPOLATION, false);

      Require(connect(&supp, SIGNAL(OnStartModule(ZXTune::Sound::Backend::Ptr, Playlist::Item::Data::Ptr)),
        SLOT(InitState(ZXTune::Sound::Backend::Ptr, Playlist::Item::Data::Ptr))));
      Require(connect(&supp, SIGNAL(OnUpdateState()), SLOT(UpdateState())));
      Require(connect(&supp, SIGNAL(OnStopModule()), SLOT(CloseState())));
    }

    virtual void InitState(ZXTune::Sound::Backend::Ptr /*player*/, Playlist::Item::Data::Ptr item)
    {
      const Playlist::Item::Capabilities caps(item);
      AYMOptions->setVisible(caps.IsAYM());
      DACOptions->setVisible(caps.IsDAC());
      SetEnabled(true);

      const Parameters::Accessor::Ptr properties = item->GetModule()->GetModuleProperties();
      if (const Parameters::Accessor::Ptr adjusted = item->GetAdjustedParameters())
      {
        AdjustedParameters = CreateMergedAccessor(adjusted, properties);
      }
      else
      {
        AdjustedParameters = properties;
      }
    }

    virtual void UpdateState()
    {
      if (isVisible() && AdjustedParameters)
      {
        //TODO: use walker?
        Parameters::IntType val;
        isYM->setEnabled(!AdjustedParameters->FindValue(Parameters::ZXTune::Core::AYM::TYPE, val));
        aymLayout->setEnabled(!AdjustedParameters->FindValue(Parameters::ZXTune::Core::AYM::LAYOUT, val));
        isDACInterpolated->setEnabled(!AdjustedParameters->FindValue(Parameters::ZXTune::Core::DAC::INTERPOLATION, val));
      }
    }

    virtual void CloseState()
    {
      SetEnabled(false);
      AdjustedParameters = Parameters::Accessor::Ptr();
    }

    //QWidget
    virtual void changeEvent(QEvent* event)
    {
      if (event && QEvent::LanguageChange == event->type())
      {
        retranslateUi(this);
      }
      ::PlaybackOptions::changeEvent(event);
    }
  private:
    void SetEnabled(bool enabled)
    {
      AYMOptions->setEnabled(enabled);
      DACOptions->setEnabled(enabled);
    }
  private:
    const Parameters::Container::Ptr Params;
    Parameters::Accessor::Ptr AdjustedParameters;
  };
}

PlaybackOptions::PlaybackOptions(QWidget& parent) : QWidget(&parent)
{
}

PlaybackOptions* PlaybackOptions::Create(QWidget& parent, PlaybackSupport& supp, Parameters::Container::Ptr params)
{
  return new PlaybackOptionsImpl(parent, supp, params);
}
