/*
Abstract:
  Conversion setup dialog

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  This file is a part of zxtune-qt application based on zxtune library
*/

#pragma once
#ifndef ZXTUNE_QT_UI_SETUP_CONVERSION_H_DEFINED
#define ZXTUNE_QT_UI_SETUP_CONVERSION_H_DEFINED

//qt includes
#include <QtGui/QDialog>

namespace UI
{
  class SetupConversionDialog : public QDialog
  {
    Q_OBJECT
  protected:
    explicit SetupConversionDialog(QWidget& parent);
  public:
    static SetupConversionDialog* Create(QWidget& parent);
  private slots:
    virtual void UpdateDescriptions() = 0;
  };
}

#endif //ZXTUNE_QT_UI_SETUP_CONVERSION_H_DEFINED
