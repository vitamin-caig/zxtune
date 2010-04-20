/*
Abstract:
  Main window creation implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  This file is a part of zxtune-qt application based on zxtune library
*/

//local includes
#include "../mainwindow.h"
#include "../playbackcontrol.h"
#include "../seekcontrol.h"
#include "main_form.h"

namespace
{
  class MainWindow : public QMainWindow, private Ui::MainForm
  {
  public:
    MainWindow()
      : PlaybackControl(QtUi::CreatePlaybackControlWidget())
      , SeekControl(QtUi::CreateSeekControlWidget())
    {
      //fill and layout
      setupUi(this);
      //add widgets to toolbars
      controlToolbar->addWidget(PlaybackControl.data());
      seekToolbar->addWidget(SeekControl.data());
    }
  private:
    QPointer<QWidget> PlaybackControl;
    QPointer<QWidget> SeekControl;
  };
};

namespace QtUi
{
  QPointer<QMainWindow> CreateMainWindow()
  {
    return QPointer<QMainWindow>(new MainWindow());
  }
}
