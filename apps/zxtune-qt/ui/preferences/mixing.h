/*
Abstract:
  Mixing settings widget interface

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  This file is a part of zxtune-qt application based on zxtune library
*/

#pragma once
#ifndef ZXTUNE_QT_UI_MIXING_H_DEFINED
#define ZXTUNE_QT_UI_MIXING_H_DEFINED

//qt includes
#include <QtGui/QWidget>

namespace UI
{
  class MixingSettingsWidget : public QWidget
  {
    Q_OBJECT
  protected:
    explicit MixingSettingsWidget(QWidget& parent);
  public:
    static MixingSettingsWidget* Create(QWidget& parent, unsigned channels);
  };
}

#endif //ZXTUNE_QT_UI_MIXING_H_DEFINED
