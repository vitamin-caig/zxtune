/**
 *
 * @file
 *
 * @brief Main window interface
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

// local includes
#include "playlist/supp/data.h"
#include "ui/mainwindow.h"
// library includes
#include <sound/backend.h>

class DesktopMainWindow : public MainWindow
{
  Q_OBJECT
public:
  static MainWindow::Ptr Create(Parameters::Container::Ptr options);

public slots:
  void SetCmdline(const QStringList& args) override = 0;

private slots:
  virtual void StartModule(Sound::Backend::Ptr, Playlist::Item::Data::Ptr) = 0;
  virtual void StopModule() = 0;

  // File menu
  virtual void ShowPreferences() = 0;
  // Help menu
  virtual void ShowComponentsInformation() = 0;
  virtual void ShowAboutProgram() = 0;
  virtual void ShowAboutQt() = 0;
  virtual void VisitHelp() = 0;
  virtual void VisitSite() = 0;
  virtual void VisitFAQ() = 0;
  virtual void ReportIssue() = 0;
  virtual void ShowError(const Error&) = 0;
};
