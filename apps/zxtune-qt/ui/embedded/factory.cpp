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
    virtual QPointer<QMainWindow> CreateMainWindow(Parameters::Container::Ptr options, const Strings::Array& cmdline) const
    {
      return QPointer<QMainWindow>(EmbeddedMainWindow::Create(options, cmdline));
    }
  };
}

WidgetsFactory& WidgetsFactory::Instance()
{
  static EmbeddedWidgetsFactory instance;
  return instance;
}
