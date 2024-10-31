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

#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtWidgets/QWidget>  // IWYU pragma: export

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
