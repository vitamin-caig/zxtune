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
#include "preferencesdialog.h"
#include "preferencesdialog.ui.h"
#include "aym.h"
#include "saa.h"
#include "z80.h"
#include "sound.h"
#include "mixing.h"
#include "plugins.h"
#include "interface.h"
#include "ui/state.h"
//common includes
#include <tools.h>
//std includes
#include <algorithm>
//boost includes
#include <boost/bind.hpp>
//qt includes
#include <QtGui/QCloseEvent>
#include <QtGui/QDialogButtonBox>
#include <QtGui/QTabWidget>
#include <QtGui/QVBoxLayout>

namespace
{
  class PreferencesDialog : public QDialog
                          , public Ui::PreferencesDialog
  {
  public:
    PreferencesDialog(QWidget& parent, bool playing)
      : QDialog(&parent)
    {
      setupUi(this);
      State = UI::State::Create(*this);
      //fill
      QWidget* const pages[] =
      {
        UI::AYMSettingsWidget::Create(*Categories),
        UI::SAASettingsWidget::Create(*Categories),
        UI::Z80SettingsWidget::Create(*Categories),
        UI::SoundSettingsWidget::Create(*Categories),
        UI::MixingSettingsWidget::Create(*Categories, 3),
        UI::MixingSettingsWidget::Create(*Categories, 4),
        UI::PluginsSettingsWidget::Create(*Categories),
        UI::InterfaceSettingsWidget::Create(*Categories)
      };
      std::for_each(pages, ArrayEnd(pages),
        boost::bind(&QTabWidget::addTab, Categories, _1, boost::bind(&QWidget::windowTitle, _1)));

      Categories->setTabEnabled(2, !playing);
      State->AddWidget(*Categories);
      State->Load();
    }

    //QWidgets virtuals
    virtual void closeEvent(QCloseEvent* event)
    {
      State->Save();
      event->accept();
    }

    virtual void changeEvent(QEvent* event)
    {
      if (event && QEvent::LanguageChange == event->type())
      {
        retranslateUi(this);
        for (int idx = 0, lim = Categories->count(); idx != lim; ++idx)
        {
          QWidget* const tab = Categories->widget(idx);
          //tab->changeEvent(event);
          Categories->setTabText(idx, tab->windowTitle());
        }
      }
      else
      {
        QDialog::changeEvent(event);
      }
    }
  private:
    UI::State::Ptr State;
  };
}

namespace UI
{
  void ShowPreferencesDialog(QWidget& parent, bool playing)
  {
    PreferencesDialog dialog(parent, playing);
    dialog.exec();
  }
}
