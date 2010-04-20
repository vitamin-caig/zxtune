/*
Abstract:
  Seek control creator declaration

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  This file is a part of zxtune-qt application based on zxtune library
*/

#ifndef ZXTUNE_QT_SEEKCONTROL_H_DEFINED
#define ZXTUNE_QT_SEEKCONTROL_H_DEFINED

//qt includes
#include <QtCore/QPointer>
#include <QtGui/QWidget>

namespace QtUi
{
  QPointer<QWidget> CreateSeekControlWidget();
}

#endif //ZXTUNE_QT_SEEKBACKCONTROL_H_DEFINED
