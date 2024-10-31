/**
 *
 * @file
 *
 * @brief UI widgets factory interface
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

// local includes
#include "apps/zxtune-qt/ui/mainwindow.h"
// library includes
#include <parameters/container.h>

class WidgetsFactory
{
public:
  virtual ~WidgetsFactory() = default;

  // main window
  virtual MainWindow::Ptr CreateMainWindow(Parameters::Container::Ptr options) const = 0;

  // singleton
  static WidgetsFactory& Instance();
};
