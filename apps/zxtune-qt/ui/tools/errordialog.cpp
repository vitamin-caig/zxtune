/*
Abstract:
  Error display widget implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  This file is a part of zxtune-qt application based on zxtune library
*/

//local includes
#include "errordialog.h"
#include "ui/utils.h"
//boost includes
#include <boost/bind.hpp>
//qt includes
#include <QtGui/QMessageBox>

namespace
{
  void CollectErrorDetails(Error::LocationRef location, Error::CodeType code, const String& text, String& resultStr)
  {
    resultStr += Error::AttributesToString(location, code, text);
  }

  String GetErrorDetails(const Error& err)
  {
    String details;
    err.WalkSuberrors(boost::bind(&CollectErrorDetails, _2, _3, _4, boost::ref(details)));
    return details;
  }
}

void ShowErrorMessage(const QString& title, const Error& err)
{
  QMessageBox msgBox;
  msgBox.setWindowTitle(QString::fromUtf8("Error"));
  msgBox.setText(title);
  msgBox.setInformativeText(ToQString(err.GetText()));
  msgBox.setDetailedText(ToQString(GetErrorDetails(err)));
  msgBox.setIcon(QMessageBox::Critical);
  msgBox.setStandardButtons(QMessageBox::Ok);
  msgBox.exec();
}
