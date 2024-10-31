/**
 *
 * @file
 *
 * @brief QT application implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "apps/zxtune-qt/singlemode.h"
#include "apps/zxtune-qt/supp/options.h"
#include "apps/zxtune-qt/ui/factory.h"
#include "apps/zxtune-qt/ui/utils.h"
#include "apps/zxtune-qt/urls.h"

#include <platform/application.h>
#include <platform/version/api.h>

#include <contract.h>

#include <QtWidgets/QApplication>

#include <utility>

namespace
{
  class QTApplication : public Platform::Application
  {
  public:
    QTApplication() = default;

    int Run(Strings::Array argv) override
    {
      int fakeArgc = 1;
      char* fakeArgv[] = {""};
      const QApplication qapp(fakeArgc, fakeArgv);
      // storageLocation(DataLocation) is ${profile}/[${organizationName}/][${applicationName}/]
      // applicationName cannot be empty since qt4.8.7 (binary name is used instead)
      // So, do not set  organization name and override application name
      qapp.setApplicationName("ZXTune");
      qapp.setApplicationVersion(ToQString(Platform::Version::GetProgramVersionString()));
      qapp.setOrganizationDomain(ToQString(Urls::Site()));
      const Parameters::Container::Ptr params = GlobalOptions::Instance().Get();
      const SingleModeDispatcher::Ptr mode = SingleModeDispatcher::Create(*params, std::move(argv));
      if (mode->StartMaster())
      {
        const MainWindow::Ptr win = WidgetsFactory::Instance().CreateMainWindow(params);
        Require(win->connect(mode, &SingleModeDispatcher::OnSlaveStarted, win, &MainWindow::SetCmdline));
        win->SetCmdline(mode->GetCmdline());
        return qapp.exec();
      }
      else
      {
        return 0;
      }
    }
  };
}  // namespace

namespace Platform
{
  namespace Version
  {
    const StringView PROGRAM_NAME = "zxtune-qt"sv;
  }

  std::unique_ptr<Application> Application::Create()
  {
    return std::unique_ptr<Application>(new QTApplication());
  }
}  // namespace Platform
