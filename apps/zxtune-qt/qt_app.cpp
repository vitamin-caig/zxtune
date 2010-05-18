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
#ifdef Q_WS_QWS
      qapp.setFont(QFont(QString::fromUtf8("Verdana")));
#endif
      //main ui
      QPointer<MainWindow> win(MainWindow::Create(argc, argv));
#ifdef Q_WS_QWS
      win->showMaximized();
#else
      win->show();
#endif
      return qapp.exec();
    }
  private:
    
  };
}

std::auto_ptr<Application> Application::Create()
{
  return std::auto_ptr<Application>(new QTApplication());
}
