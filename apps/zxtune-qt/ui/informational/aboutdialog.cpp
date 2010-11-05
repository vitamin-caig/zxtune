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
#include <formatter.h>
//text includes
#include "text/text.h"

namespace
{
  class AboutDialogImpl : public AboutDialog
                        , private Ui::AboutDialog
  {
  public:
    explicit AboutDialogImpl(QWidget* /*parent*/)
    {
      //do not set parent
      setupUi(this);
      const String appVersion = GetProgramVersionString();
      const String feedbackText = (Formatter(Text::FEEDBACK_TEXT) % appVersion).str();
      setWindowTitle(ToQString(appVersion));
      feedbackLabel->setText(ToQString(feedbackText));
    }

    virtual void Show()
    {
      exec();
    }
  };
}

AboutDialog* AboutDialog::Create(QWidget* parent)
{
  return new AboutDialogImpl(parent);
}
