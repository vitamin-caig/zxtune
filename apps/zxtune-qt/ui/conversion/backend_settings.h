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

#include <parameters/container.h>

#include <string_view.h>
#include <types.h>

#include <QtWidgets/QWidget>

namespace UI
{
  class BackendSettingsWidget : public QWidget
  {
    Q_OBJECT
  protected:
    explicit BackendSettingsWidget(QWidget& parent);

  public:
    // TODO: use Sound::BackendId
    virtual StringView GetBackendId() const = 0;
    virtual QString GetDescription() const = 0;

    template<class T>
    void SettingChanged(T)
    {
      emit SettingsChanged();
    }
  signals:
    void SettingsChanged();
  };
}  // namespace UI
