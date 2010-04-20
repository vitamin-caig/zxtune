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

//local includes
#include "seek_controls_ui.h"

class SeekControls : public QWidget
                   , private Ui::SeekControls
{
  Q_OBJECT
public:
  SeekControls(QWidget* parent = 0);
};

#endif //ZXTUNE_QT_SEEKBACKCONTROL_H_DEFINED
