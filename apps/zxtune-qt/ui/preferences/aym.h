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

//qt includes
#include <QtGui/QWidget>

namespace UI
{
  class AYMSettingsWidget : public QWidget
  {
    Q_OBJECT
  protected:
    explicit AYMSettingsWidget(QWidget& parent);
  public:
    static AYMSettingsWidget* Create(QWidget& parent);
  private slots:
    virtual void OnClockRateChanged(const QString& val) = 0;
    virtual void OnClockRatePresetChanged(int idx) = 0;
  };
}
