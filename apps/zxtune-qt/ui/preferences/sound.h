/**
 *
 * @file
 *
 * @brief Sound settings pane interface
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include <QtWidgets/QWidget>

namespace UI
{
  class SoundSettingsWidget : public QWidget
  {
    Q_OBJECT
  protected:
    explicit SoundSettingsWidget(QWidget& parent);

  public:
    static SoundSettingsWidget* Create(QWidget& parent);
  };
}  // namespace UI
