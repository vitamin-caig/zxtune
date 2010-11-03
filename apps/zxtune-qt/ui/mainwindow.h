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

//library includes
#include <core/module_player.h>
//qt includes
#include <QtCore/QPointer>
#include <QtGui/QMainWindow>

class MainWindow : public QMainWindow
{
  Q_OBJECT
public:
  static QPointer<MainWindow> Create(int argc, char* argv[]);

public slots:
  virtual void StartModule(ZXTune::Module::Player::ConstPtr) = 0;
  virtual void StopModule() = 0;

  virtual void ShowAboutQt() = 0;
};

#endif //ZXTUNE_QT_MAINWINDOW_H_DEFINED
