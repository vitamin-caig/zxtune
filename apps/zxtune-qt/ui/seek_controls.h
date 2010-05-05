/*
Abstract:
  Seek control widget declaration

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  This file is a part of zxtune-qt application based on zxtune library
*/

#ifndef ZXTUNE_QT_SEEKCONTROL_H_DEFINED
#define ZXTUNE_QT_SEEKCONTROL_H_DEFINED

//library includes
#include <core/module_types.h>
//qt includes
#include <QtGui/QWidget>

class SeekControls : public QWidget
{
  Q_OBJECT
public:
  //creator
  static SeekControls* Create(QWidget* parent);

public slots:
  virtual void InitState(const ZXTune::Module::Information&) = 0;
  virtual void UpdateState(uint, const ZXTune::Module::Tracking&, const ZXTune::Module::Analyze::ChannelsState&) = 0;
  virtual void CloseState(const ZXTune::Module::Information&) = 0;
signals:
  void OnSeeking(int);
};

#endif //ZXTUNE_QT_SEEKBACKCONTROL_H_DEFINED
