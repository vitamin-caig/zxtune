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

#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtWidgets/QWidget>  // IWYU pragma: export

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
