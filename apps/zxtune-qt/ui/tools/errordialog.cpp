/**
* 
* @file
*
* @brief Error dialog implementation
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "errordialog.h"
#include "ui/utils.h"
//qt includes
#include <QtGui/QMessageBox>

void ShowErrorMessage(const QString& title, const Error& err)
{
  QMessageBox msgBox;
  msgBox.setWindowTitle(QMessageBox::tr("Error"));
  const QString& errorText = ToQStringFromLocal(err.GetText());
  if (title.size() != 0)
  {
    msgBox.setText(title);
    msgBox.setInformativeText(errorText);
  }
  else
  {
    msgBox.setText(errorText);
  }
  msgBox.setDetailedText(ToQStringFromLocal(err.ToString()));
  msgBox.setIcon(QMessageBox::Critical);
  msgBox.setStandardButtons(QMessageBox::Ok);
  msgBox.exec();
}
