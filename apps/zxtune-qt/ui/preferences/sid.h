/**
 *
 * @file
 *
 * @brief SID settings pane interface
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
  class SIDSettingsWidget : public QWidget
  {
    Q_OBJECT
  protected:
    explicit SIDSettingsWidget(QWidget& parent);

  public:
    static SIDSettingsWidget* Create(QWidget& parent);
  };
}  // namespace UI
