/**
 *
 * @file
 *
 * @brief Plugins settings pane interface
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include <QtWidgets/QWidget>

namespace UI
{
  class PluginsSettingsWidget : public QWidget
  {
    Q_OBJECT
  protected:
    explicit PluginsSettingsWidget(QWidget& parent);

  public:
    static PluginsSettingsWidget* Create(QWidget& parent);
  };
}  // namespace UI
