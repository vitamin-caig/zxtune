/*
Abstract:
  Overlay progress widget

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  This file is a part of zxtune-qt application based on zxtune library
*/

#pragma once
#ifndef ZXTUNE_QT_OVERLAY_PROGRESS_H_DEFINED
#define ZXTUNE_QT_OVERLAY_PROGRESS_H_DEFINED

//qt includes
#include <QtGui/QWidget>

class OverlayProgress : public QWidget
{
  Q_OBJECT
protected:
  explicit OverlayProgress(QWidget& parent);
public:
  //creator
  static OverlayProgress* Create(QWidget& parent);
public slots:
  virtual void UpdateProgress(int progress) = 0;
signals:
  void Canceled();
};

#endif //ZXTUNE_QT_OVERLAY_PROGRESS_H_DEFINED
