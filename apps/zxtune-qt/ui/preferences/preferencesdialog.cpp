/**
 *
 * @file
 *
 * @brief Preferences dialog implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "apps/zxtune-qt/ui/preferences/preferencesdialog.h"

#include "apps/zxtune-qt/ui/preferences/aym.h"
#include "apps/zxtune-qt/ui/preferences/interface.h"
#include "apps/zxtune-qt/ui/preferences/mixing.h"
#include "apps/zxtune-qt/ui/preferences/plugins.h"
#include "apps/zxtune-qt/ui/preferences/saa.h"
#include "apps/zxtune-qt/ui/preferences/sid.h"
#include "apps/zxtune-qt/ui/preferences/sound.h"
#include "apps/zxtune-qt/ui/preferences/z80.h"
#include "apps/zxtune-qt/ui/state.h"
#include "preferencesdialog.ui.h"

#include <QtGui/QCloseEvent>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QVBoxLayout>

#include <algorithm>
#include <utility>

namespace
{
  class PreferencesDialog
    : public QDialog
    , public Ui::PreferencesDialog
  {
  public:
    PreferencesDialog(QWidget& parent, bool playing)
      : QDialog(&parent)
    {
      setupUi(this);
      State = UI::State::Create(*this);
      // fill
      QWidget* const soundSettingsPage = UI::SoundSettingsWidget::Create(*Categories);
      QWidget* const pages[] = {UI::AYMSettingsWidget::Create(*Categories),
                                UI::SAASettingsWidget::Create(*Categories),
                                UI::SIDSettingsWidget::Create(*Categories),
                                UI::Z80SettingsWidget::Create(*Categories),
                                soundSettingsPage,
                                UI::MixingSettingsWidget::Create(*Categories, 3),
                                UI::MixingSettingsWidget::Create(*Categories, 4),
                                UI::PluginsSettingsWidget::Create(*Categories),
                                UI::InterfaceSettingsWidget::Create(*Categories)};
      for (auto* p : pages)
      {
        Categories->addTab(p, p->windowTitle());
      }
      Categories->setTabEnabled(std::find(pages, std::end(pages), soundSettingsPage) - pages, !playing);
      State->AddWidget(*Categories);
      State->Load();
    }

    // QWidgets virtuals
    void closeEvent(QCloseEvent* event) override
    {
      State->Save();
      event->accept();
    }

    void changeEvent(QEvent* event) override
    {
      if (event && QEvent::LanguageChange == event->type())
      {
        retranslateUi(this);
        for (int idx = 0, lim = Categories->count(); idx != lim; ++idx)
        {
          QWidget* const tab = Categories->widget(idx);
          // tab->changeEvent(event);
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
}  // namespace

namespace UI
{
  void ShowPreferencesDialog(QWidget& parent, bool playing)
  {
    PreferencesDialog dialog(parent, playing);
    dialog.exec();
  }
}  // namespace UI
