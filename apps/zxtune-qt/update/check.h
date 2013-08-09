/*
Abstract:
  Update checking logic interface

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  This file is a part of zxtune-qt application based on zxtune library
*/

#pragma once
#ifndef ZXTUNE_QT_UPDATE_CHECK_DEFINED
#define ZXTUNE_QT_UPDATE_CHECK_DEFINED

//qt includes
#include <QtGui/QWidget>

class Error;

namespace Update
{
  class CheckOperation : public QObject
  {
    Q_OBJECT
  public:
    static CheckOperation* Create(QWidget& parent);
  public slots:
    virtual void Execute() = 0;
  private slots:
    virtual void ExecuteBackground() = 0;
  signals:
    void ErrorOccurred(const Error&);
  };
};

#endif //ZXTUNE_QT_UPDATE_CHECK_DEFINED
