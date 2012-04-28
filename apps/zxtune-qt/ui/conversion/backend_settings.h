/*
Abstract:
  Abstract backend settings widget

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  This file is a part of zxtune-qt application based on zxtune library
*/

#pragma once
#ifndef ZXTUNE_QT_UI_BACKEND_SETTINGS_H_DEFINED
#define ZXTUNE_QT_UI_BACKEND_SETTINGS_H_DEFINED

//common includes
#include <types.h>
#include <parameters.h>
//qt includes
#include <QtGui/QWidget>

namespace UI
{
  class BackendSettingsWidget : public QWidget
  {
    Q_OBJECT
  protected:
    explicit BackendSettingsWidget(QWidget& parent);
  public:
    virtual Parameters::Container::Ptr GetSettings() const = 0;

    virtual String GetBackendId() const = 0;
    virtual QString GetDescription() const = 0;
  signals:
    void SettingsChanged();
  };
}

#endif //ZXTUNE_QT_UI_BACKEND_SETTINGS_H_DEFINED
