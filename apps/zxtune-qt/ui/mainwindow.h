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

#include <QtCore/QPointer>
#include <QtWidgets/QMainWindow>  // IWYU pragma: export

class MainWindow : public QMainWindow
{
public:
  using Ptr = QPointer<MainWindow>;

  virtual void SetCmdline(const QStringList& args) = 0;
};
