/**
 *
 * @file
 *
 * @brief OSS settings pane implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include "apps/zxtune-qt/ui/preferences/sound_oss.h"
#include "apps/zxtune-qt/supp/options.h"
#include "apps/zxtune-qt/ui/tools/parameters_helpers.h"
#include "sound_oss.ui.h"
// common includes
#include <contract.h>
// library includes
#include <sound/backends_parameters.h>
// qt includes
#include <QtWidgets/QFileDialog>
// std includes
#include <utility>

namespace
{
  class OssOptionsWidget
    : public UI::OssSettingsWidget
    , public UI::Ui_OssSettingsWidget
  {
  public:
    explicit OssOptionsWidget(QWidget& parent)
      : UI::OssSettingsWidget(parent)
      , Options(GlobalOptions::Instance().Get())
    {
      // setup self
      setupUi(this);

      Require(connect(selectDevice, &QToolButton::clicked, this, &OssOptionsWidget::DeviceSelected));
      Require(connect(selectMixer, &QToolButton::clicked, this, &OssOptionsWidget::MixerSelected));

      using namespace Parameters::ZXTune::Sound::Backends::Oss;
      Parameters::StringValue::Bind(*device, *Options, DEVICE, DEVICE_DEFAULT);
      Parameters::StringValue::Bind(*mixer, *Options, MIXER, MIXER_DEFAULT);
    }

    StringView GetBackendId() const override
    {
      return "oss"sv;
    }

    QString GetDescription() const override
    {
      return nameGroup->title();
    }

    // QWidget
    void changeEvent(QEvent* event) override
    {
      if (event && QEvent::LanguageChange == event->type())
      {
        retranslateUi(this);
      }
      UI::OssSettingsWidget::changeEvent(event);
    }

  private:
    void DeviceSelected()
    {
      QString devFile = device->text();
      if (OpenFileDialog(UI::OssSettingsWidget::tr("Select device"), devFile))
      {
        device->setText(devFile);
      }
    }

    void MixerSelected()
    {
      QString mixFile = mixer->text();
      if (OpenFileDialog(UI::OssSettingsWidget::tr("Select mixer"), mixFile))
      {
        mixer->setText(mixFile);
      }
    }

    bool OpenFileDialog(const QString& title, QString& filename)
    {
      // do not use UI::OpenSingleFileDialog for keeping global settings intact
      QFileDialog dialog(this, title, filename, QLatin1String("*"));
      dialog.setFileMode(QFileDialog::ExistingFile);
      dialog.setOption(QFileDialog::ReadOnly, true);
      dialog.setOption(QFileDialog::DontUseNativeDialog, true);
      dialog.setOption(QFileDialog::HideNameFilterDetails, true);
      if (QDialog::Accepted == dialog.exec())
      {
        filename = dialog.selectedFiles().front();
        return true;
      }
      return false;
    }

  private:
    const Parameters::Container::Ptr Options;
  };
}  // namespace
namespace UI
{
  OssSettingsWidget::OssSettingsWidget(QWidget& parent)
    : BackendSettingsWidget(parent)
  {}

  BackendSettingsWidget* OssSettingsWidget::Create(QWidget& parent)
  {
    return new OssOptionsWidget(parent);
  }
}  // namespace UI
