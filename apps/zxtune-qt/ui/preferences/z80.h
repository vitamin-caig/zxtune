/*
Abstract:
  Z80 settings widget interface

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  This file is a part of zxtune-qt application based on zxtune library
*/

#pragma once
#ifndef ZXTUNE_QT_UI_Z80_H_DEFINED
#define ZXTUNE_QT_UI_Z80_H_DEFINED

//qt includes
#include <QtGui/QWidget>

namespace UI
{
  class Z80SettingsWidget : public QWidget
  {
    Q_OBJECT
  protected:
    explicit Z80SettingsWidget(QWidget& parent);
  public:
    static Z80SettingsWidget* Create(QWidget& parent);
  };
}

#endif //ZXTUNE_QT_UI_Z80_H_DEFINED
