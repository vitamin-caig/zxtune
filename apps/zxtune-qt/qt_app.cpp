/*
Abstract:
  QT application implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  This file is a part of zxtune-qt application based on zxtune library
*/

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

inline void InitResources()
{
  Q_INIT_RESOURCE(icons);
}

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
      InitResources();
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
