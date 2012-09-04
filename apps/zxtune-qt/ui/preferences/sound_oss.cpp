/*
Abstract:
  Oss settings widget implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  This file is a part of zxtune-qt application based on zxtune library
*/

//local includes
#include "sound_oss.h"
#include "sound_oss.ui.h"
#include "supp/options.h"
#include "ui/tools/parameters_helpers.h"
//common includes
#include <contract.h>
#include <tools.h>
//library includes
#include <sound/backends_parameters.h>
//qt includes
#include <QtGui/QFileDialog>

namespace
{
  class OssOptionsWidget : public UI::OssSettingsWidget
                         , public Ui::OssOptions
  {
  public:
    explicit OssOptionsWidget(QWidget& parent)
      : UI::OssSettingsWidget(parent)
      , Options(GlobalOptions::Instance().Get())
    {
      //setup self
      setupUi(this);

      connect(selectDevice, SIGNAL(clicked()), SLOT(DeviceSelected()));
      connect(selectMixer, SIGNAL(clicked()), SLOT(MixerSelected()));

      using namespace Parameters::ZXTune::Sound::Backends::Oss;
      Parameters::StringValue::Bind(*device, *Options, DEVICE, DEVICE_DEFAULT);
      Parameters::StringValue::Bind(*mixer, *Options, MIXER, MIXER_DEFAULT);
    }

    virtual Parameters::Container::Ptr GetSettings() const
    {
      //TODO
      return Parameters::Container::Ptr();
    }

    virtual String GetBackendId() const
    {
      static const Char ID[] = {'o', 's', 's', '\0'};
      return ID;
    }

    virtual QString GetDescription() const
    {
      return nameGroup->title();
    }

    virtual void DeviceSelected()
    {
      QString devFile = device->text();
      if (OpenFileDialog(tr("Select device", "OssOptions"), devFile))
      {
        device->setText(devFile);
      }
    }

    virtual void MixerSelected()
    {
      QString mixFile = mixer->text();
      if (OpenFileDialog(tr("Select mixer", "OssOptions"), mixFile))
      {
        mixer->setText(mixFile);
      }
    }
  private:
    bool OpenFileDialog(const QString& title, QString& filename)
    {
      //do not use UI::OpenSingleFileDialog for keeping global settings intact
      QFileDialog dialog(this, title, filename, QLatin1String("*"));
      dialog.setFileMode(QFileDialog::ExistingFile);
      dialog.setReadOnly(true);
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
}
namespace UI
{
  OssSettingsWidget::OssSettingsWidget(QWidget& parent)
    : BackendSettingsWidget(parent)
  {
  }

  BackendSettingsWidget* OssSettingsWidget::Create(QWidget& parent)
  {
    return new OssOptionsWidget(parent);
  }
}
