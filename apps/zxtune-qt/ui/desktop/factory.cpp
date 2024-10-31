/**
 *
 * @file
 *
 * @brief WidgetsFactory implementation for desktop
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include "apps/zxtune-qt/ui/factory.h"
#include "apps/zxtune-qt/ui/desktop/mainwindow.h"
// std includes
#include <utility>

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
}  // namespace

WidgetsFactory& WidgetsFactory::Instance()
{
  static DesktopWidgetsFactory instance;
  return instance;
}
