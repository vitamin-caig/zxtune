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

#include "apps/zxtune-qt/ui/mainwindow.h"  // IWYU pragma: export

#include "parameters/container.h"

#include <QtCore/QObject>
#include <QtCore/QString>

class DesktopMainWindow : public MainWindow
{
  Q_OBJECT
public:
  static MainWindow::Ptr Create(Parameters::Container::Ptr options);
};
