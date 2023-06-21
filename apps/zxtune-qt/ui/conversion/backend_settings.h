/**
 *
 * @file
 *
 * @brief Backend settings base widget interface
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

// common includes
#include <parameters/container.h>
#include <types.h>
// qt includes
#include <QtWidgets/QWidget>

namespace UI
{
  class BackendSettingsWidget : public QWidget
  {
    Q_OBJECT
  protected:
    explicit BackendSettingsWidget(QWidget& parent);

  public:
    virtual String GetBackendId() const = 0;
    virtual QString GetDescription() const = 0;

  protected:
    template<class T>
    void SettingChanged(T)
    {
      emit SettingsChanged();
    }
  signals:
    void SettingsChanged();
  };
}  // namespace UI
