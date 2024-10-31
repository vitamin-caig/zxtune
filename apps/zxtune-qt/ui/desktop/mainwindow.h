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

// local includes
#include "apps/zxtune-qt/ui/mainwindow.h"
// library includes
#include <parameters/container.h>

class DesktopMainWindow : public MainWindow
{
  Q_OBJECT
public:
  static MainWindow::Ptr Create(Parameters::Container::Ptr options);
};
