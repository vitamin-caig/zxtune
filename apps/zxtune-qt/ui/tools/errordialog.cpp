/**
 *
 * @file
 *
 * @brief Error dialog implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include "apps/zxtune-qt/ui/tools/errordialog.h"
#include "apps/zxtune-qt/ui/utils.h"
// qt includes
#include <QtWidgets/QMessageBox>
// std includes
#include <utility>

void ShowErrorMessage(const QString& title, const Error& err)
{
  QMessageBox msgBox;
  msgBox.setWindowTitle(QMessageBox::tr("Error"));
  const QString& errorText = ToQString(err.GetText());
  if (title.size() != 0)
  {
    msgBox.setText(title);
    msgBox.setInformativeText(errorText);
  }
  else
  {
    msgBox.setText(errorText);
  }
  msgBox.setDetailedText(ToQString(err.ToString()));
  msgBox.setIcon(QMessageBox::Critical);
  msgBox.setStandardButtons(QMessageBox::Ok);
  msgBox.exec();
}
