/**
* 
* @file
*
* @brief WidgetsFactory implementation for desktop
*
* @author vitamin.caig@gmail.com
*
**/

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
