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
#include "ui/mainwindow.h"
#include <apps/base/app.h>
//qt includes
#include <QtGui/QApplication>

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
      MainWindow win(argc, argv);
      win.show();
      return qapp.exec();
    }
  private:
    
  };
}

std::auto_ptr<Application> Application::Create()
{
  return std::auto_ptr<Application>(new QTApplication());
}
