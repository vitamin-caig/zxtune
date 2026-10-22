/**
 *
 * @file
 *
 * @brief OGG settings widget implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "apps/zxtune-qt/ui/conversion/ogg_settings.h"

#include "apps/zxtune-qt/supp/options.h"
#include "apps/zxtune-qt/ui/conversion/backend_settings.h"
#include "apps/zxtune-qt/ui/tools/parameters_helpers.h"
#include "apps/zxtune-qt/ui/utils.h"
#include "ogg_settings.ui.h"

#include "sound/backends/parameters.h"

#include "contract.h"

#include <utility>

namespace
{
  QString Translate(const char* msg)
  {
    return QApplication::translate("OggSettings", msg);
  }

}  // namespace

namespace UI
{
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
      Value::BindExclusive(*selectQuality, *Options, ZXTune::Sound::Backends::Ogg::Mode::Key,
                           ZXTune::Sound::Backends::Ogg::Mode::Quality);
      Value::Bind<ZXTune::Sound::Backends::Ogg::Quality>(*qualityValue, *Options);
      Value::BindExclusive(*selectBitrate, *Options, ZXTune::Sound::Backends::Ogg::Mode::Key,
                           ZXTune::Sound::Backends::Ogg::Mode::ABR);
      Value::Bind<ZXTune::Sound::Backends::Ogg::Bitrate>(*bitrateValue, *Options);
      // fixup
      if (!selectQuality->isChecked() && !selectBitrate->isChecked())
      {
        selectQuality->setChecked(true);
      }
    }

    StringView GetBackendId() const override
    {
      return "ogg"sv;
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
  BackendSettingsWidget* CreateOGGSettingsWidget(QWidget& parent)
  {
    return new OGGSettingsWidget(parent);
  }
}  // namespace UI
