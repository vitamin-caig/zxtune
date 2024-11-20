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
#include "ogg_settings.ui.h"

#include "parameters/container.h"
#include "parameters/identifier.h"
#include "sound/backends_parameters.h"

#include "contract.h"
#include "string_view.h"

#include <QtCore/QString>
#include <QtCore/QtCore>
#include <QtWidgets/QApplication>
#include <QtWidgets/QRadioButton>
#include <QtWidgets/QSlider>

#include <memory>

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
}  // namespace

namespace UI
{
  BackendSettingsWidget* CreateOGGSettingsWidget(QWidget& parent)
  {
    return new OGGSettingsWidget(parent);
  }
}  // namespace UI
