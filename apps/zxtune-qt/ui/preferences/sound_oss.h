/**
 *
 * @file
 *
 * @brief OSS settings pane interface
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

// local includes
#include "../conversion/backend_settings.h"

namespace UI
{
  class OssSettingsWidget : public BackendSettingsWidget
  {
    Q_OBJECT
  protected:
    explicit OssSettingsWidget(QWidget& parent);

  public:
    static BackendSettingsWidget* Create(QWidget& parent);
  private slots:
    virtual void DeviceSelected() = 0;
    virtual void MixerSelected() = 0;
  };
}  // namespace UI
