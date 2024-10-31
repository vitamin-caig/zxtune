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

#include "apps/zxtune-qt/ui/conversion/backend_settings.h"

namespace UI
{
  class OssSettingsWidget : public BackendSettingsWidget
  {
    Q_OBJECT
  protected:
    explicit OssSettingsWidget(QWidget& parent);

  public:
    static BackendSettingsWidget* Create(QWidget& parent);
  };
}  // namespace UI
