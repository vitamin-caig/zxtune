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
#include "ui/factory.h"
#include "ui/utils.h"
#include "supp/options.h"
#include <apps/base/app.h>
#include <apps/version/api.h>
//qt includes
#include <QtGui/QApplication>
#include <QtGui/QDesktopServices>
//text includes
#include "text/text.h"

namespace
{
  class QTApplication : public Application
  {
  public:
    QTApplication()
    {
    }

    virtual int Run(int argc, char* argv[])
    {
      QApplication qapp(argc, argv);
      //main ui
      Strings::Array cmdline(argc - 1);
      std::transform(argv + 1, argv + argc, cmdline.begin(), &FromStdString);
      const QPointer<QMainWindow> win = WidgetsFactory::Instance().CreateMainWindow(GlobalOptions::Instance().Get(), cmdline);
      qapp.setOrganizationName(QLatin1String(Text::PROJECT_NAME));
      qapp.setOrganizationDomain(QLatin1String(Text::PROGRAM_SITE));
      qapp.setApplicationVersion(ToQString(GetProgramVersionString()));
      //std::cout << QDesktopServices::storageLocation(QDesktopServices::DataLocation).toStdString() << std::endl;
      return qapp.exec();
    }
  };
}

std::auto_ptr<Application> Application::Create()
{
  return std::auto_ptr<Application>(new QTApplication());
}
