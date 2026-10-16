/**
 *
 * @file
 *
 * @brief MP3 settings widget implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "apps/zxtune-qt/ui/conversion/mp3_settings.h"

#include "apps/zxtune-qt/supp/options.h"
#include "apps/zxtune-qt/ui/conversion/backend_settings.h"
#include "apps/zxtune-qt/ui/tools/parameters_helpers.h"
#include "apps/zxtune-qt/ui/utils.h"
#include "mp3_settings.ui.h"

#include "sound/backends_parameters.h"

#include "contract.h"

#include <utility>

namespace
{
  QString Translate(const char* msg)
  {
    return QApplication::translate("Mp3Settings", msg);
  }

  const Strings::Array CHANNEL_MODES = {Parameters::ZXTune::Sound::Backends::Mp3::CHANNELS_DEFAULT,
                                        Parameters::ZXTune::Sound::Backends::Mp3::CHANNELS_STEREO,
                                        Parameters::ZXTune::Sound::Backends::Mp3::CHANNELS_JOINTSTEREO,
                                        Parameters::ZXTune::Sound::Backends::Mp3::CHANNELS_MONO};

}  // namespace

namespace UI
{
  class MP3SettingsWidget
    : public UI::BackendSettingsWidget
    , private Ui::Mp3Settings
  {
  public:
    explicit MP3SettingsWidget(QWidget& parent)
      : UI::BackendSettingsWidget(parent)
      , Options(GlobalOptions::Instance().Get())
    {
      // setup self
      setupUi(this);

      Require(connect(selectCBR, &QRadioButton::toggled, this, &UI::BackendSettingsWidget::SettingChanged<bool>));
      Require(connect(selectABR, &QRadioButton::toggled, this, &UI::BackendSettingsWidget::SettingChanged<bool>));
      Require(connect(bitrateValue, &QSlider::valueChanged, this, &UI::BackendSettingsWidget::SettingChanged<int>));
      Require(connect(selectQuality, &QRadioButton::toggled, this, &UI::BackendSettingsWidget::SettingChanged<bool>));
      Require(connect(qualityValue, &QSlider::valueChanged, this, &UI::BackendSettingsWidget::SettingChanged<int>));
      Require(connect(channelsMode, qOverload<int>(&QComboBox::currentIndexChanged), this,
                      &UI::BackendSettingsWidget::SettingChanged<int>));

      using namespace Parameters;
      Value::BindExclusive(*selectCBR, *Options, ZXTune::Sound::Backends::Mp3::MODE,
                           ZXTune::Sound::Backends::Mp3::MODE_CBR);
      Value::BindExclusive(*selectABR, *Options, ZXTune::Sound::Backends::Mp3::MODE,
                           ZXTune::Sound::Backends::Mp3::MODE_ABR);
      Value::BindExclusive(*selectQuality, *Options, ZXTune::Sound::Backends::Mp3::MODE,
                           ZXTune::Sound::Backends::Mp3::MODE_VBR);
      Value::Bind(*bitrateValue, *Options, ZXTune::Sound::Backends::Mp3::BITRATE,
                  ZXTune::Sound::Backends::Mp3::BITRATE_DEFAULT);
      Value::Bind(*qualityValue, *Options, ZXTune::Sound::Backends::Mp3::QUALITY,
                  ZXTune::Sound::Backends::Mp3::QUALITY_DEFAULT);
      Value::Bind(*channelsMode, *Options, ZXTune::Sound::Backends::Mp3::CHANNELS, CHANNEL_MODES,
                  ZXTune::Sound::Backends::Mp3::CHANNELS_DEFAULT);
      // fixup
      if (!selectCBR->isChecked() && !selectABR->isChecked() && !selectQuality->isChecked())
      {
        selectCBR->setChecked(true);
      }
    }

    StringView GetBackendId() const override
    {
      return "mp3"sv;
    }

    QString GetDescription() const override
    {
      QString descr = GetBitrateDescription();
      if (0 != channelsMode->currentIndex())
      {
        descr += QLatin1String(", ");
        descr += channelsMode->currentText();
      }
      return descr;
    }

  private:
    QString GetBitrateDescription() const
    {
      if (selectCBR->isChecked() || selectABR->isChecked())
      {
        const int bitrate = bitrateValue->value();
        return selectCBR->isChecked() ? Translate(QT_TRANSLATE_NOOP("Mp3Settings", "%1 kbps")).arg(bitrate)
                                      : Translate(QT_TRANSLATE_NOOP("Mp3Settings", "~ %1 kbps")).arg(bitrate);
      }
      else if (selectQuality->isChecked())
      {
        return Translate(QT_TRANSLATE_NOOP("Mp3Settings", "Quality %1")).arg(qualityValue->value());
      }
      else
      {
        return {};
      }
    }

  private:
    const Parameters::Container::Ptr Options;
  };
  BackendSettingsWidget* CreateMP3SettingsWidget(QWidget& parent)
  {
    return new MP3SettingsWidget(parent);
  }
}  // namespace UI
