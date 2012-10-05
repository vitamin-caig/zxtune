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
//qt includes
#include <QtGui/QApplication>
#include <QtGui/QDialog>
//text includes
#include "text/text.h"

namespace
{
  const char* const FEEDBACK_FORMAT = QT_TRANSLATE_NOOP("AboutDialog", "<a href=\"mailto:%1?subject=Feedback for %2\">Send feedback</a>");

  class AboutDialog : public QDialog
                    , private Ui::AboutDialog
  {
  public:
    explicit AboutDialog(QWidget& parent)
      : QDialog(&parent)
    {
      //do not set parent
      setupUi(this);
      const QString appVersion(ToQString(GetProgramVersionString()));
      buildLabel->setText(appVersion);
      const QString feedbackFormat(QApplication::translate("AboutDialog", FEEDBACK_FORMAT, 0, QApplication::UnicodeUTF8));
      feedbackLabel->setText(feedbackFormat.arg(QLatin1String(Text::FEEDBACK_EMAIL)).arg(appVersion));
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
