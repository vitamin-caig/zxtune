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
#include "playlist/supp/data.h"
#include "ui/mainwindow.h"
// library includes
#include <sound/backend.h>

class DesktopMainWindow : public MainWindow
{
  Q_OBJECT
public:
  static MainWindow::Ptr Create(Parameters::Container::Ptr options);
};
