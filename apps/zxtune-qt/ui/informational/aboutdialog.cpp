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
#include <apps/base/app.h>
//common includes
#include <format.h>
//text includes
#include "text/text.h"

namespace
{
  class AboutDialogImpl : public AboutDialog
                        , private Ui::AboutDialog
  {
  public:
    explicit AboutDialogImpl(QWidget& parent)
      : ::AboutDialog(parent)
    {
      //do not set parent
      setupUi(this);
      const String appVersion = GetProgramVersionString();
      buildLabel->setText(ToQString(appVersion));
      const String feedbackText = Strings::Format(Text::FEEDBACK_TEXT, appVersion);
      feedbackLabel->setText(ToQString(feedbackText));
    }

    virtual void Show()
    {
      exec();
    }
  };
}

AboutDialog::AboutDialog(QWidget& parent) : QDialog(&parent)
{
}

AboutDialog* AboutDialog::Create(QWidget& parent)
{
  return new AboutDialogImpl(parent);
}
