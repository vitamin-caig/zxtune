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
#include "z80.h"
//common includes
#include <tools.h>
//std includes
#include <algorithm>
//boost includes
#include <boost/bind.hpp>
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
      QWidget* const pages[] =
      {
        UI::AYMSettingsWidget::Create(*tabs),
        UI::Z80SettingsWidget::Create(*tabs)
      };
      std::for_each(pages, ArrayEnd(pages),
        boost::bind(&QTabWidget::addTab, tabs, _1, boost::bind(&QWidget::windowTitle, _1)));
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
