/*
Abstract:
  SAA settings widget interface

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  This file is a part of zxtune-qt application based on zxtune library
*/

#pragma once
#ifndef ZXTUNE_QT_UI_SAA_H_DEFINED
#define ZXTUNE_QT_UI_SAA_H_DEFINED

//qt includes
#include <QtGui/QWidget>

namespace UI
{
  class SAASettingsWidget : public QWidget
  {
    Q_OBJECT
  protected:
    explicit SAASettingsWidget(QWidget& parent);
  public:
    static SAASettingsWidget* Create(QWidget& parent);
  };
}

#endif //ZXTUNE_QT_UI_SAA_H_DEFINED
