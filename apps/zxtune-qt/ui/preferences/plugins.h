/**
* 
* @file
*
* @brief Plugins settings pane interface
*
* @author vitamin.caig@gmail.com
*
**/

#pragma once

//qt includes
#include <QtGui/QWidget>

namespace UI
{
  class PluginsSettingsWidget : public QWidget
  {
    Q_OBJECT
  protected:
    explicit PluginsSettingsWidget(QWidget& parent);
  public:
    static PluginsSettingsWidget* Create(QWidget& parent);
  private slots:
    virtual void OnDurationTypeChanged(int row) = 0;
    virtual void OnDurationValueChanged(int duration) = 0;
    virtual void OnDurationDefault() = 0;
  };
}
