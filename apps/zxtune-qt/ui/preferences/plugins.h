/*
Abstract:
  Plugins settings widget interface

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  This file is a part of zxtune-qt application based on zxtune library
*/

#pragma once
#ifndef ZXTUNE_QT_UI_PLUGINS_H_DEFINED
#define ZXTUNE_QT_UI_PLUGINS_H_DEFINED

//qt includes
#include <QtGui/QWidget>

namespace UI
{
  class PluginsSettingsWidget : public QWidget
  {
    Q_OBJECT
  protected:
    explicit PluginsSettingsWidget(QWidget& parent);
  public:
    static PluginsSettingsWidget* Create(QWidget& parent);
  };
}

#endif //ZXTUNE_QT_UI_PLUGINS_H_DEFINED
