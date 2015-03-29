/**
* 
* @file
*
* @brief Main window interface
*
* @author vitamin.caig@gmail.com
*
**/

#pragma once

//local includes
#include "ui/mainwindow.h"
//library includes
#include <parameters/container.h>

class EmbeddedMainWindow : public MainWindow
{
  Q_OBJECT
public:
  static MainWindow::Ptr Create(Parameters::Container::Ptr options);

public slots:
  virtual void SetCmdline(const QStringList& args) = 0;
};
