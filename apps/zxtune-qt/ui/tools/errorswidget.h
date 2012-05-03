/*
Abstract:
  Errors widget interface

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  This file is a part of zxtune-qt application based on zxtune library
*/

#pragma once
#ifndef ZXTUNE_QT_ERRORSWIDGET_H_DEFINED
#define ZXTUNE_QT_ERRORSWIDGET_H_DEFINED

//common includes
#include <error.h>
//common includes
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
}

#endif //ZXTUNE_QT_ERRORSWIDGET_H_DEFINED
