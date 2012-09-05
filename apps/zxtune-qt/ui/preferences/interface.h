/*
Abstract:
  Interface settings widget interface

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  This file is a part of zxtune-qt application based on zxtune library
*/

#pragma once
#ifndef ZXTUNE_QT_UI_INTERFACE_H_DEFINED
#define ZXTUNE_QT_UI_INTERFACE_H_DEFINED

//qt includes
#include <QtGui/QWidget>

namespace UI
{
  class InterfaceSettingsWidget : public QWidget
  {
    Q_OBJECT
  protected:
    explicit InterfaceSettingsWidget(QWidget& parent);
  public:
    static InterfaceSettingsWidget* Create(QWidget& parent);
  private slots:
    virtual void OnLanguageChanged(int idx) = 0;
  };
}

#endif //ZXTUNE_QT_UI_INTERFACE_H_DEFINED
