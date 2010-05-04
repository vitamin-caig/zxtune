/*
Abstract:
  Main window declaration

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  This file is a part of zxtune-qt application based on zxtune library
*/

#ifndef ZXTUNE_QT_MAINWINDOW_H_DEFINED
#define ZXTUNE_QT_MAINWINDOW_H_DEFINED

//qt includes
#include <QtGui/QMainWindow>
#include <QtCore/QPointer>


class MainWindow : public QMainWindow
{
  Q_OBJECT
public:
  static QPointer<MainWindow> Create(int arg, char* argv[]);
};

#endif //ZXTUNE_QT_MAINWINDOW_H_DEFINED
