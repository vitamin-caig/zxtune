/**
* 
* @file
*
* @brief WidgetsFactory implementation for embedded systems
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "ui/factory.h"
#include "mainwindow.h"

namespace
{
  class EmbeddedWidgetsFactory : public WidgetsFactory
  {
  public:
    MainWindow::Ptr CreateMainWindow(Parameters::Container::Ptr options) const override
    {
      return EmbeddedMainWindow::Create(options);
    }
  };
}

WidgetsFactory& WidgetsFactory::Instance()
{
  static EmbeddedWidgetsFactory instance;
  return instance;
}
