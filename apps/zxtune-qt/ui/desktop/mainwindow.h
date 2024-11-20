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

#include "apps/zxtune-qt/ui/mainwindow.h"

#include "parameters/container.h"

class DesktopMainWindow : public MainWindow
{
  Q_OBJECT
public:
  static MainWindow::Ptr Create(Parameters::Container::Ptr options);
};
