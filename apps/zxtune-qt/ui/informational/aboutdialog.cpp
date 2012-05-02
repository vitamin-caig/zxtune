/*
Abstract:
  About dialog implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  This file is a part of zxtune-qt application based on zxtune library
*/

//local includes
#include "aboutdialog.h"
#include "aboutdialog.ui.h"
#include "ui/utils.h"
#include <apps/version/api.h>
//common includes
#include <format.h>
//qt includes
#include <QtGui/QDialog>
//text includes
#include "text/text.h"

namespace
{
  class AboutDialog : public QDialog
                    , private Ui::AboutDialog
  {
  public:
    explicit AboutDialog(QWidget& parent)
      : QDialog(&parent)
    {
      //do not set parent
      setupUi(this);
      const String appVersion = GetProgramVersionString();
      buildLabel->setText(ToQString(appVersion));
      const String feedbackText = Strings::Format(Text::FEEDBACK_TEXT, appVersion);
      feedbackLabel->setText(ToQString(feedbackText));
    }
  };
}

namespace UI
{
  void ShowProgramInformation(QWidget& parent)
  {
    AboutDialog dialog(parent);
    dialog.exec();
  }
}
