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

#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtWidgets/QWidget>  // IWYU pragma: export

class Error;

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
