/**
 *
 * @file
 *
 * @brief OGG settings widget implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include "ogg_settings.h"
#include "ogg_settings.ui.h"
#include "supp/options.h"
#include "ui/tools/parameters_helpers.h"
#include "ui/utils.h"
// common includes
#include <contract.h>
// library includes
#include <sound/backends_parameters.h>
// std includes
#include <utility>

namespace
{
  QString Translate(const char* msg)
  {
    return QApplication::translate("OggSettings", msg);
  }

  class OGGSettingsWidget
    : public UI::BackendSettingsWidget
    , private Ui::OggSettings
  {
  public:
    explicit OGGSettingsWidget(QWidget& parent)
      : UI::BackendSettingsWidget(parent)
      , Options(GlobalOptions::Instance().Get())
    {
      // setup self
      setupUi(this);

      Require(connect(selectQuality, &QRadioButton::toggled, this, &UI::BackendSettingsWidget::SettingChanged<bool>));
      Require(connect(qualityValue, &QSlider::valueChanged, this, &UI::BackendSettingsWidget::SettingChanged<int>));
      Require(connect(selectBitrate, &QRadioButton::toggled, this, &UI::BackendSettingsWidget::SettingChanged<bool>));
      Require(connect(bitrateValue, &QSlider::valueChanged, this, &UI::BackendSettingsWidget::SettingChanged<int>));

      using namespace Parameters;
      ExclusiveValue::Bind(*selectQuality, *Options, ZXTune::Sound::Backends::Ogg::MODE,
                           ZXTune::Sound::Backends::Ogg::MODE_QUALITY);
      IntegerValue::Bind(*qualityValue, *Options, ZXTune::Sound::Backends::Ogg::QUALITY,
                         ZXTune::Sound::Backends::Ogg::QUALITY_DEFAULT);
      ExclusiveValue::Bind(*selectBitrate, *Options, ZXTune::Sound::Backends::Ogg::MODE,
                           ZXTune::Sound::Backends::Ogg::MODE_ABR);
      IntegerValue::Bind(*bitrateValue, *Options, ZXTune::Sound::Backends::Ogg::BITRATE,
                         ZXTune::Sound::Backends::Ogg::BITRATE_DEFAULT);
      // fixup
      if (!selectQuality->isChecked() && !selectBitrate->isChecked())
      {
        selectQuality->setChecked(true);
      }
    }

    String GetBackendId() const override
    {
      static const Char ID[] = {'o', 'g', 'g', '\0'};
      return ID;
    }

    QString GetDescription() const override
    {
      if (selectQuality->isChecked())
      {
        return Translate(QT_TRANSLATE_NOOP("OggSettings", "Quality %1")).arg(qualityValue->value());
      }
      else if (selectBitrate->isChecked())
      {
        return Translate(QT_TRANSLATE_NOOP("OggSettings", "Average bitrate %1 kbps")).arg(bitrateValue->value());
      }
      else
      {
        return {};
      }
    }

  private:
    const Parameters::Container::Ptr Options;
  };
}  // namespace

namespace UI
{
  BackendSettingsWidget* CreateOGGSettingsWidget(QWidget& parent)
  {
    return new OGGSettingsWidget(parent);
  }
}  // namespace UI
