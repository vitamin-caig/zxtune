/**
 *
 * @file
 *
 * @brief Win32 settings pane interface
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

// local includes
#include "../conversion/backend_settings.h"

namespace UI
{
  class Win32SettingsWidget : public BackendSettingsWidget
  {
    Q_OBJECT
  protected:
    explicit Win32SettingsWidget(QWidget& parent);

  public:
    static BackendSettingsWidget* Create(QWidget& parent);
  };
}  // namespace UI
