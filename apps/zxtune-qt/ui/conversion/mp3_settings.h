/*
Abstract:
  MP3 settings widget

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  This file is a part of zxtune-qt application based on zxtune library
*/

#pragma once
#ifndef ZXTUNE_QT_UI_MP3_SETTINGS_H_DEFINED
#define ZXTUNE_QT_UI_MP3_SETTINGS_H_DEFINED

//common includes
#include <types.h>
//qt includes
#include <QtGui/QWidget>

namespace UI
{
  class MP3SettingsWidget : public QWidget
  {
    Q_OBJECT
  protected:
    explicit MP3SettingsWidget(QWidget& parent);
  public:
    static MP3SettingsWidget* Create(QWidget& parent);

    virtual String GetBackendId() const = 0;
    virtual QString GetDescription() const = 0;
  signals:
    void SettingsChanged();
  };
}

#endif //ZXTUNE_QT_UI_MP3_SETTINGS_H_DEFINED
