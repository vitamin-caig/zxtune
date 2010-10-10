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
#include "aboutdialog_ui.h"
#include "aboutdialog_moc.h"

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
