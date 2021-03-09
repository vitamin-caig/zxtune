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

// qt includes
#include <QtGui/QWidget>

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
