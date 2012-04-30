/*
Abstract:
  Preferences dialog implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  This file is a part of zxtune-qt application based on zxtune library
*/

//local includes
#include "setup_preferences.h"
#include "aym.h"
//qt includes
#include <QtGui/QDialogButtonBox>
#include <QtGui/QTabWidget>
#include <QtGui/QVBoxLayout>

namespace
{
  class PreferencesDialog : public QDialog
  {
  public:
    explicit PreferencesDialog(QWidget& parent)
      : QDialog(&parent)
    {
      QDialogButtonBox* const buttons = new QDialogButtonBox(QDialogButtonBox::Ok, Qt::Horizontal, this);
      this->connect(buttons, SIGNAL(accepted()), this, SLOT(accept()));
      QVBoxLayout* const layout = new QVBoxLayout(this);
      QTabWidget* const tabs = new QTabWidget(this);
      layout->addWidget(tabs);
      layout->addWidget(buttons);
      //fill
      if (QWidget* aym = UI::AYMSettingsWidget::Create(*tabs))
      {
        tabs->addTab(aym, aym->windowTitle());
      }
      setWindowTitle(tr("Preferences"));
    }
  };
}

namespace UI
{
  QDialog* CreatePreferencesDialog(QWidget& parent)
  {
    return new PreferencesDialog(parent);
  }

  void ShowPreferencesDialog(QWidget& parent)
  {
    PreferencesDialog dialog(parent);
    dialog.exec();
  }
}
