/**
 *
 * @file
 *
 * @brief Mixing settings pane interface
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

// qt includes
#include <QtWidgets/QWidget>

namespace UI
{
  class MixingSettingsWidget : public QWidget
  {
    Q_OBJECT
  protected:
    explicit MixingSettingsWidget(QWidget& parent);

  public:
    static MixingSettingsWidget* Create(QWidget& parent, unsigned channels);
  };
}  // namespace UI
