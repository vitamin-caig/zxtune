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

//qt includes
#include <QtGui/QWidget>

namespace UI
{
  class SoundSettingsWidget : public QWidget
  {
    Q_OBJECT
  protected:
    explicit SoundSettingsWidget(QWidget& parent);
  public:
    static SoundSettingsWidget* Create(QWidget& parent);
  private slots:
    virtual void ChangeSoundFrequency(int idx) = 0;
    virtual void SelectBackend(int idx) = 0;
    virtual void MoveBackendUp() = 0;
    virtual void MoveBackendDown() = 0;
  };
}
