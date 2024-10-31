/**
 *
 * @file
 *
 * @brief Mixing settings pane implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "apps/zxtune-qt/ui/preferences/mixing.h"

#include "apps/zxtune-qt/supp/options.h"
#include "apps/zxtune-qt/ui/preferences/mixer.h"
#include "apps/zxtune-qt/ui/utils.h"
#include "mixing.ui.h"

#include <sound/mixer_parameters.h>
#include <sound/sample.h>

#include <QtWidgets/QLabel>

#include <utility>

namespace
{
  const char* INPUT_CHANNEL_NAMES[4] = {"A", "B", "C", "D"};

  class MixingOptionsWidget
    : public UI::MixingSettingsWidget
    , public UI::Ui_MixingSettingsWidget
  {
  public:
    MixingOptionsWidget(QWidget& parent, unsigned channels)
      : UI::MixingSettingsWidget(parent)
      , Options(GlobalOptions::Instance().Get())
      , Channels(channels)
    {
      // setup self
      setupUi(this);
      SetTitle();

      for (uint_t inChan = 0; inChan != channels; ++inChan)
      {
        channelNames->addWidget(new QLabel(QLatin1String(INPUT_CHANNEL_NAMES[inChan]), this));
        for (uint_t outChan = 0; outChan != Sound::Sample::CHANNELS; ++outChan)
        {
          UI::MixerWidget* const mixer = UI::MixerWidget::Create(*this, static_cast<UI::MixerWidget::Channel>(outChan));
          channelValues->addWidget(mixer);
          const auto name = Parameters::ZXTune::Sound::Mixer::LEVEL(channels, inChan, outChan);
          const auto defVal = Parameters::ZXTune::Sound::Mixer::LEVEL_DEFAULT(channels, inChan, outChan);
          Parameters::MixerValue::Bind(*mixer, *Options, name, defVal);
        }
      }
    }

    // QWidget
    void changeEvent(QEvent* event) override
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
}  // namespace

namespace UI
{
  MixingSettingsWidget::MixingSettingsWidget(QWidget& parent)
    : QWidget(&parent)
  {}

  MixingSettingsWidget* MixingSettingsWidget::Create(QWidget& parent, unsigned channels)
  {
    return new MixingOptionsWidget(parent, channels);
  }
}  // namespace UI
