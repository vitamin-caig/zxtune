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
    MainWindow::Ptr CreateMainWindow(Parameters::Container::Ptr options) const override
    {
      return DesktopMainWindow::Create(options);
    }
  };
}

WidgetsFactory& WidgetsFactory::Instance()
{
  static DesktopWidgetsFactory instance;
  return instance;
}
