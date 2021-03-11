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

// common includes
#include <error.h>
// common includes
#include <QtGui/QWidget>

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
  public slots:
    virtual void AddError(const Error& err) = 0;
  private slots:
    virtual void Previous() = 0;
    virtual void Next() = 0;
    virtual void Dismiss() = 0;
    virtual void DismissAll() = 0;
  };
}  // namespace UI
