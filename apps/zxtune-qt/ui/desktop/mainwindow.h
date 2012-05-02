/*
Abstract:
  Main window declaration

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  This file is a part of zxtune-qt application based on zxtune library
*/

#pragma once
#ifndef ZXTUNE_QT_MAINWINDOW_H_DEFINED
#define ZXTUNE_QT_MAINWINDOW_H_DEFINED

//local includes
#include "playlist/supp/data.h"
//library includes
#include <sound/backend.h>
//qt includes
#include <QtCore/QPointer>
#include <QtGui/QMainWindow>

class MainWindow : public QMainWindow
{
  Q_OBJECT
public:
  static QPointer<MainWindow> Create(Parameters::Container::Ptr options, const StringArray& cmdline);

public slots:
  virtual void StartModule(ZXTune::Sound::Backend::Ptr, Playlist::Item::Data::Ptr) = 0;
  virtual void StopModule() = 0;

  //File menu
  virtual void ShowPreferences() = 0;
  //Help menu
  virtual void ShowComponentsInformation() = 0;
  virtual void ShowAboutProgram() = 0;
  virtual void ShowAboutQt() = 0;
  virtual void VisitHelp() = 0;
  virtual void VisitSite() = 0;
  virtual void VisitFAQ() = 0;
  virtual void ReportIssue() = 0;
};

#endif //ZXTUNE_QT_MAINWINDOW_H_DEFINED
