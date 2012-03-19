/*
Abstract:
  Supported formats widget

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  This file is a part of zxtune-qt application based on zxtune library
*/

#pragma once
#ifndef ZXTUNE_QT_UI_SUPPORTED_FORMATS_H_DEFINED
#define ZXTUNE_QT_UI_SUPPORTED_FORMATS_H_DEFINED

//common includes
#include <types.h>
//qt includes
#include <QtGui/QWidget>

namespace UI
{
  class SupportedFormatsWidget : public QWidget
  {
    Q_OBJECT
  protected:
    explicit SupportedFormatsWidget(QWidget& parent);
  public:
    static SupportedFormatsWidget* Create(QWidget& parent);

    virtual String GetSelectedId() const = 0;
    virtual QString GetDescription() const = 0;
  signals:
    void SettingsChanged();
  };
}

#endif //ZXTUNE_QT_UI_SUPPORTED_FORMATS_H_DEFINED
