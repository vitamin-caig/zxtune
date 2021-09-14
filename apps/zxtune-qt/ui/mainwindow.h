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

// qt includes
#include <QtCore/QPointer>
#include <QtWidgets/QMainWindow>

class MainWindow : public QMainWindow
{
public:
  typedef QPointer<MainWindow> Ptr;

public slots:
  virtual void SetCmdline(const QStringList& args) = 0;
};
