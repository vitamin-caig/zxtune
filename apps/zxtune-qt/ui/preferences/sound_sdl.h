/**
 *
 * @file
 *
 * @brief SDL settings pane interface
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include "apps/zxtune-qt/ui/conversion/backend_settings.h"  // IWYU pragma: export

#include <QtCore/QObject>
#include <QtCore/QString>

namespace UI
{
  class SdlSettingsWidget : public BackendSettingsWidget
  {
    Q_OBJECT
  protected:
    explicit SdlSettingsWidget(QWidget& parent);

  public:
    static BackendSettingsWidget* Create(QWidget& parent);
  };
}  // namespace UI
