/*
Abstract:
  AYM settings widget interface

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  This file is a part of zxtune-qt application based on zxtune library
*/

#pragma once
#ifndef ZXTUNE_QT_UI_AYM_H_DEFINED
#define ZXTUNE_QT_UI_AYM_H_DEFINED

//qt includes
#include <QtGui/QWidget>

namespace UI
{
  class AYMSettingsWidget : public QWidget
  {
    Q_OBJECT
  protected:
    explicit AYMSettingsWidget(QWidget& parent);
  public:
    static AYMSettingsWidget* Create(QWidget& parent);
  private slots:
    virtual void OnClockRateChanged(const QString& val) = 0;
    virtual void OnClockRatePresetChanged(int idx) = 0;
  };
}

#endif //ZXTUNE_QT_UI_AYM_H_DEFINED
