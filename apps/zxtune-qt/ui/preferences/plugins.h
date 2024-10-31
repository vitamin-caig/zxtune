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

#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtWidgets/QWidget>  // IWYU pragma: export

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
