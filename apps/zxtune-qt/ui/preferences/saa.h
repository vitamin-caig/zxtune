/**
 *
 * @file
 *
 * @brief SAA settings pane interface
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include <QtWidgets/QWidget>

namespace UI
{
  class SAASettingsWidget : public QWidget
  {
    Q_OBJECT
  protected:
    explicit SAASettingsWidget(QWidget& parent);

  public:
    static SAASettingsWidget* Create(QWidget& parent);
  };
}  // namespace UI
