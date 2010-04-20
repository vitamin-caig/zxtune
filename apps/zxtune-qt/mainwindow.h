/*
Abstract:
  Main window creator declaration

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  This file is a part of zxtune-qt application based on zxtune library
*/

#ifndef ZXTUNE_QT_MAINWINDOW_H_DEFINED
#define ZXTUNE_QT_MAINWINDOW_H_DEFINED

//qt includes
#include <QtCore/QPointer>
#include <QtGui/QMainWindow>

namespace QtUi
{
  QPointer<QMainWindow> CreateMainWindow();
}

#endif //ZXTUNE_QT_MAINWINDOW_H_DEFINED
