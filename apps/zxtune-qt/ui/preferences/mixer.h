/*
Abstract:
  Single channel mixer widget interface

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  This file is a part of zxtune-qt application based on zxtune library
*/

#pragma once
#ifndef ZXTUNE_QT_UI_MIXER_H_DEFINED
#define ZXTUNE_QT_UI_MIXER_H_DEFINED

//common includes
#include <parameters.h>
//qt includes
#include <QtGui/QWidget>

namespace UI
{
  class MixerWidget : public QWidget
  {
    Q_OBJECT
  protected:
    explicit MixerWidget(QWidget& parent);
  public:
    static MixerWidget* Create(QWidget& parent, const QString& name);
  public slots:
    virtual void setValue(int val) = 0;
  signals:
    void valueChanged(int val);
  };
}

namespace Parameters
{
  class MixerValue : public QObject
  {
    Q_OBJECT
  protected:
    explicit MixerValue(QObject& parent);
  public:
    static void Bind(UI::MixerWidget& mix, Container& ctr, const NameType& name, int defValue);
  private slots:
    virtual void SetValue(int value) = 0;
  };
}

#endif //ZXTUNE_QT_UI_MIXER_H_DEFINED
