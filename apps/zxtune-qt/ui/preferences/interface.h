/**
 *
 * @file
 *
 * @brief UI settings pane interface
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include <QtWidgets/QWidget>

namespace UI
{
  class InterfaceSettingsWidget : public QWidget
  {
    Q_OBJECT
  protected:
    explicit InterfaceSettingsWidget(QWidget& parent);

  public:
    static InterfaceSettingsWidget* Create(QWidget& parent);
  };
}  // namespace UI
