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
#include "supp/mixer.h"
#include "supp/options.h"
#include "ui/utils.h"
//qt includes
#include <QtGui/QLabel>
//text includes
#include "text/text.h"

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

      for (int inChan = 0; inChan != channels; ++inChan)
      {
        channelNames->addWidget(new QLabel(QLatin1String(INPUT_CHANNEL_NAMES[inChan]), this));
        for (uint_t outChan = 0; outChan != ZXTune::Sound::OUTPUT_CHANNELS; ++outChan)
        {
          UI::MixerWidget* const mixer = UI::MixerWidget::Create(*this, static_cast<UI::MixerWidget::Channel>(outChan));
          channelValues->addWidget(mixer);
          const int defLevel = GetMixerChannelDefaultValue(channels, inChan, outChan);
          Parameters::MixerValue::Bind(*mixer, *Options, GetMixerChannelParameterName(channels, inChan, outChan), defLevel);
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
