/**
 *
 * @file
 *
 * @brief FLAC settings widget implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include "flac_settings.h"
#include "flac_settings.ui.h"
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

    String GetBackendId() const override
    {
      static const Char ID[] = {'f', 'l', 'a', 'c', '\0'};
      return ID;
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
