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

//local includes
#include "mainwindow_ui.h"

class MainWindow : public QMainWindow
                 , private Ui::MainWindow
{
  Q_OBJECT
public:
  MainWindow();

public:
  class PlaybackControls* Controls;
  class SeekControls* Seeking;
};

#endif //ZXTUNE_QT_MAINWINDOW_H_DEFINED
