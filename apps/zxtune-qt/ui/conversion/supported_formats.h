/**
 *
 * @file
 *
 * @brief Supported formats pane interface
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

// common includes
#include <types.h>
// library includes
#include <strings/array.h>
// qt includes
#include <QtGui/QWidget>

namespace UI
{
  class SupportedFormatsWidget : public QWidget
  {
    Q_OBJECT
  protected:
    explicit SupportedFormatsWidget(QWidget& parent);

  public:
    static SupportedFormatsWidget* Create(QWidget& parent);

    virtual String GetSelectedId() const = 0;
    virtual QString GetDescription() const = 0;

    static Strings::Array GetSoundTypes();
  signals:
    void SettingsChanged();
  };
}  // namespace UI
