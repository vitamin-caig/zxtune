/*
Abstract:
  Mixing settings widget implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  This file is a part of zxtune-qt application based on zxtune library
*/

//local includes
#include "mixing.h"
#include "mixing.ui.h"
#include "mixer.h"
#include "supp/options.h"
#include "ui/utils.h"
//library includes
#include <sound/mixer_parameters.h>
#include <sound/sample.h>
//qt includes
#include <QtGui/QLabel>

namespace
{
  const char* INPUT_CHANNEL_NAMES[4] =
  {
    "A",
    "B",
    "C",
    "D"
  };

  class MixingOptionsWidget : public UI::MixingSettingsWidget
                            , public UI::Ui_MixingSettingsWidget
  {
  public:
    MixingOptionsWidget(QWidget& parent, unsigned channels)
      : UI::MixingSettingsWidget(parent)
      , Options(GlobalOptions::Instance().Get())
      , Channels(channels)
    {
      //setup self
      setupUi(this);
      SetTitle();

      for (uint_t inChan = 0; inChan != channels; ++inChan)
      {
        channelNames->addWidget(new QLabel(QLatin1String(INPUT_CHANNEL_NAMES[inChan]), this));
        for (uint_t outChan = 0; outChan != Sound::Sample::CHANNELS; ++outChan)
        {
          UI::MixerWidget* const mixer = UI::MixerWidget::Create(*this, static_cast<UI::MixerWidget::Channel>(outChan));
          channelValues->addWidget(mixer);
          const Parameters::NameType name = Parameters::ZXTune::Sound::Mixer::LEVEL(channels, inChan, outChan);
          const int defVal = Parameters::ZXTune::Sound::Mixer::LEVEL_DEFAULT(channels, inChan, outChan);
          Parameters::MixerValue::Bind(*mixer, *Options, name, defVal);
        }
      }
    }

    //QWidget
    virtual void changeEvent(QEvent* event)
    {
      if (event && QEvent::LanguageChange == event->type())
      {
        SetTitle();
      }
      UI::MixingSettingsWidget::changeEvent(event);
    }
  private:
    void SetTitle()
    {
      setWindowTitle(UI::MixingSettingsWidget::tr("%1-channels mixer").arg(Channels));
    }
  private:
    const Parameters::Container::Ptr Options;
    const int Channels;
  };
}

namespace UI
{
  MixingSettingsWidget::MixingSettingsWidget(QWidget& parent) : QWidget(&parent)
  {
  }

  MixingSettingsWidget* MixingSettingsWidget::Create(QWidget& parent, unsigned channels)
  {
    return new MixingOptionsWidget(parent, channels);
  }
}
