/*
Abstract:
  Status control widget declaration

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  This file is a part of zxtune-qt application based on zxtune library
*/

#ifndef ZXTUNE_QT_STATUSCONTROL_H_DEFINED
#define ZXTUNE_QT_STATUSCONTROL_H_DEFINED

//library includes
#include <core/module_player.h>
//qt includes
#include <QtGui/QWidget>

class StatusControl : public QWidget
{
  Q_OBJECT
public:
  //creator
  static StatusControl* Create(QWidget* parent);

public slots:
  virtual void InitState(ZXTune::Module::Player::ConstPtr) = 0;
  virtual void UpdateState() = 0;
  virtual void CloseState() = 0;
};

#endif //ZXTUNE_QT_STATUSCONTROL_H_DEFINED
