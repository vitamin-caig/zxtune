/**
 *
 * @file
 *
 * @brief Errors widget interface
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include <error.h>

#include <QtWidgets/QWidget>

namespace UI
{
  class ErrorsWidget : public QWidget
  {
  public:
    Q_OBJECT
  protected:
    explicit ErrorsWidget(QWidget& parent);

  public:
    static ErrorsWidget* Create(QWidget& parent);
    virtual void AddError(const Error& err) = 0;
  };
}  // namespace UI
