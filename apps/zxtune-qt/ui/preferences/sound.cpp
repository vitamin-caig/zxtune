/*
Abstract:
  Sound settings widget implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  This file is a part of zxtune-qt application based on zxtune library
*/

//local includes
#include "sound.h"
#include "sound.ui.h"
#include "supp/options.h"
#include "ui/utils.h"
#include "ui/tools/parameters_helpers.h"
//common includes
#include <contract.h>
#include <tools.h>
//library includes
#include <sound/sound_parameters.h>
//boost includes
#include <boost/bind.hpp>

namespace
{
  static const uint_t FREQUENCES[] =
  {
    8000,
    12000,
    16000,
    22000,
    24000,
    32000,
    44100,
    48000
  };
  
  class SoundOptionsWidget : public UI::SoundSettingsWidget
                           , public Ui::SoundOptions
  {
  public:
    SoundOptionsWidget(QWidget& parent, bool playing)
      : UI::SoundSettingsWidget(parent)
      , Options(GlobalOptions::Instance().Get())
    {
      //setup self
      setupUi(this);
      frameDurationValue->setDisabled(playing);
      soundFrequency->setDisabled(playing);
      
      Parameters::IntegerValue::Bind(*frameDurationValue, *Options, Parameters::ZXTune::Sound::FRAMEDURATION, Parameters::ZXTune::Sound::FRAMEDURATION_DEFAULT);
      Parameters::IntType freq = Parameters::ZXTune::Sound::FREQUENCY_DEFAULT;
      Options->FindIntValue(Parameters::ZXTune::Sound::FREQUENCY, freq);
      FillFrequences();
      SetFrequency(freq);
      connect(soundFrequency, SIGNAL(currentIndexChanged(int)), SLOT(ChangeSoundFrequency(int)));
    }

    virtual void ChangeSoundFrequency(int idx)
    {
      const qlonglong val = FREQUENCES[idx];
      Options->SetIntValue(Parameters::ZXTune::Sound::FREQUENCY, val);
    }
  private:
    void FillFrequences()
    {
      std::for_each(FREQUENCES, ArrayEnd(FREQUENCES), boost::bind(&SoundOptionsWidget::AddFrequency, this, _1));
    }
    
    void SetFrequency(uint_t val)
    {
      const uint_t* const frq = std::find(FREQUENCES, ArrayEnd(FREQUENCES), val);
      if (frq != ArrayEnd(FREQUENCES))
      {
        soundFrequency->setCurrentIndex(frq - FREQUENCES);
      }
    }
    
    void AddFrequency(uint_t freq)
    {
      const QString txt = QString("%1 Hz").arg(freq);
      soundFrequency->addItem(txt);
    }
  private:
    const Parameters::Container::Ptr Options;
  };
}
namespace UI
{
  SoundSettingsWidget::SoundSettingsWidget(QWidget& parent)
    : QWidget(&parent)
  {
  }

  SoundSettingsWidget* SoundSettingsWidget::Create(QWidget& parent, bool playing)
  {
    return new SoundOptionsWidget(parent, playing);
  }
}
