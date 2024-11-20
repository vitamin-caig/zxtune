/**
 *
 * @file
 *
 * @brief AYM settings pane interface
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include <QtWidgets/QWidget>

namespace UI
{
  class AYMSettingsWidget : public QWidget
  {
    Q_OBJECT
  protected:
    explicit AYMSettingsWidget(QWidget& parent);

  public:
    static AYMSettingsWidget* Create(QWidget& parent);
  };
}  // namespace UI
