/**
 *
 * @file
 *
 * @brief Z80 settings pane interface
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include <QtWidgets/QWidget>

namespace UI
{
  class Z80SettingsWidget : public QWidget
  {
    Q_OBJECT
  protected:
    explicit Z80SettingsWidget(QWidget& parent);

  public:
    static Z80SettingsWidget* Create(QWidget& parent);
  };
}  // namespace UI
