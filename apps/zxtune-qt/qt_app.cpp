/**
* 
* @file
*
* @brief QT application implementation
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "singlemode.h"
#include "ui/factory.h"
#include "ui/utils.h"
#include "supp/options.h"
//common includes
#include <contract.h>
//library includes
#include <platform/application.h>
#include <platform/version/api.h>
//qt includes
#include <QtGui/QApplication>
//text includes
#include "text/text.h"

namespace
{
  class QTApplication : public Platform::Application
  {
  public:
    QTApplication()
    {
    }

    virtual int Run(int argc, const char* argv[])
    {
      QApplication qapp(argc, const_cast<char**>(argv));
      qapp.setOrganizationName(QLatin1String(Text::PROJECT_NAME));
      qapp.setOrganizationDomain(QLatin1String(Text::PROGRAM_SITE));
      qapp.setApplicationVersion(ToQString(Platform::Version::GetProgramVersionString()));
      const Parameters::Container::Ptr params = GlobalOptions::Instance().Get();
      const SingleModeDispatcher::Ptr mode = SingleModeDispatcher::Create(params, argc - 1, argv + 1);
      if (mode->StartMaster()) {
        const MainWindow::Ptr win = WidgetsFactory::Instance().CreateMainWindow(params);
        Require(win->connect(mode, SIGNAL(OnSlaveStarted(const QStringList&)), SLOT(SetCmdline(const QStringList&))));
        win->SetCmdline(mode->GetCmdline());
        return qapp.exec();
      } else {
        return 0;
      }
    }
  };
}

namespace Platform
{
  std::auto_ptr<Application> Application::Create()
  {
    return std::auto_ptr<Application>(new QTApplication());
  }
}
