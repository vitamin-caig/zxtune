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
#include <apps/base/app.h>
#include <apps/base/parsing.h>
//qt includes
#include <QtGui/QApplication>

inline void InitResources()
{
  Q_INIT_RESOURCE(icons);
}

namespace
{
  Parameters::Container::Ptr OpenConfigFile()
  {
    const Parameters::Container::Ptr result = Parameters::Container::Create();
    ParseConfigFile(String(), *result);
    return result;
  }

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
      const Parameters::Container::Ptr options = OpenConfigFile();
      StringArray cmdline(argc - 1);
      std::transform(argv + 1, argv + argc, cmdline.begin(), &FromStdString);
      const QPointer<QMainWindow> win = WidgetsFactory::Instance().CreateMainWindow(options, cmdline);
      return qapp.exec();
    }
  };
}

std::auto_ptr<Application> Application::Create()
{
  return std::auto_ptr<Application>(new QTApplication());
}
