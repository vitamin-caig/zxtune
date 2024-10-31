/**
 *
 * @file
 *
 * @brief FLAC settings widget implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "apps/zxtune-qt/ui/conversion/flac_settings.h"

#include "apps/zxtune-qt/supp/options.h"
#include "apps/zxtune-qt/ui/conversion/backend_settings.h"
#include "apps/zxtune-qt/ui/tools/parameters_helpers.h"
#include "flac_settings.ui.h"

#include "parameters/container.h"
#include "parameters/identifier.h"
#include "sound/backends_parameters.h"

#include "contract.h"
#include "string_view.h"

#include <QtCore/QString>
#include <QtCore/QtCore>
#include <QtWidgets/QApplication>
#include <QtWidgets/QSlider>

#include <memory>

namespace
{
  QString Translate(const char* msg)
  {
    return QApplication::translate("FlacSettings", msg);
  }

  class FLACSettingsWidget
    : public UI::BackendSettingsWidget
    , private Ui::FlacSettings
  {
  public:
    explicit FLACSettingsWidget(QWidget& parent)
      : UI::BackendSettingsWidget(parent)
      , Options(GlobalOptions::Instance().Get())
    {
      // setup self
      setupUi(this);

      Require(connect(compressionValue, &QSlider::valueChanged, this, &UI::BackendSettingsWidget::SettingChanged<int>));

      using namespace Parameters;
      IntegerValue::Bind(*compressionValue, *Options, ZXTune::Sound::Backends::Flac::COMPRESSION,
                         ZXTune::Sound::Backends::Flac::COMPRESSION_DEFAULT);
    }

    StringView GetBackendId() const override
    {
      return "flac"sv;
    }

    QString GetDescription() const override
    {
      return Translate(QT_TRANSLATE_NOOP("FlacSettings", "Compression %1")).arg(compressionValue->value());
    }

  private:
    const Parameters::Container::Ptr Options;
  };
}  // namespace

namespace UI
{
  BackendSettingsWidget* CreateFLACSettingsWidget(QWidget& parent)
  {
    return new FLACSettingsWidget(parent);
  }
}  // namespace UI
