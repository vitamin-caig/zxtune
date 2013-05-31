/*
Abstract:
  Analyzer control widget declaration

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  This file is a part of zxtune-qt application based on zxtune library
*/

#pragma once
#ifndef ZXTUNE_QT_ANALYZERCONTROL_H_DEFINED
#define ZXTUNE_QT_ANALYZERCONTROL_H_DEFINED

//library includes
#include <sound/backend.h>
//qt includes
#include <QtGui/QWidget>

class PlaybackSupport;

class AnalyzerControl : public QWidget
{
  Q_OBJECT
protected:
  explicit AnalyzerControl(QWidget& parent);
public:
  //creator
  static AnalyzerControl* Create(QWidget& parent, PlaybackSupport& supp);

public slots:
  virtual void InitState(Sound::Backend::Ptr) = 0;
  virtual void UpdateState() = 0;
  virtual void CloseState() = 0;
};

#endif //ZXTUNE_QT_SEEKBACKCONTROL_H_DEFINED
