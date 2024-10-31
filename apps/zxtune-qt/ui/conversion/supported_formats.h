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

#include "strings/array.h"

#include "string_view.h"

#include <QtWidgets/QWidget>

namespace UI
{
  class SupportedFormatsWidget : public QWidget
  {
    Q_OBJECT
  protected:
    explicit SupportedFormatsWidget(QWidget& parent);

  public:
    static SupportedFormatsWidget* Create(QWidget& parent);

    virtual StringView GetSelectedId() const = 0;
    virtual QString GetDescription() const = 0;

    static Strings::Array GetSoundTypes();
  signals:
    void SettingsChanged();
  };
}  // namespace UI
