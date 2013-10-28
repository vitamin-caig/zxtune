/*
Abstract:
  UI widgets factory implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  This file is a part of zxtune-qt application based on zxtune library
*/

//local includes
#include "ui/factory.h"
#include "mainwindow.h"

namespace
{
  class DesktopWidgetsFactory : public WidgetsFactory
  {
  public:
    virtual QPointer<QMainWindow> CreateMainWindow(Parameters::Container::Ptr options, const Strings::Array& cmdline) const
    {
      return QPointer<QMainWindow>(MainWindow::Create(options, cmdline));
    }
  };
}

WidgetsFactory& WidgetsFactory::Instance()
{
  static DesktopWidgetsFactory instance;
  return instance;
}
