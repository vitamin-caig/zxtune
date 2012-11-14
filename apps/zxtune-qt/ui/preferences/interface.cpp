/*
Abstract:
  Interface settings widget implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  This file is a part of zxtune-qt application based on zxtune library
*/

//local includes
#include "interface.h"
#include "interface.ui.h"
#include "playlist/parameters.h"
#include "supp/options.h"
#include "ui/utils.h"
#include "ui/parameters.h"
#include "ui/desktop/language.h"
#include "ui/tools/parameters_helpers.h"
//common includes
#include <contract.h>
//qt includes
#include <QtGui/QRadioButton>

namespace
{
  class InterfaceOptionsWidget : public UI::InterfaceSettingsWidget
                               , public UI::Ui_InterfaceSettingsWidget
  {
  public:
    explicit InterfaceOptionsWidget(QWidget& parent)
      : UI::InterfaceSettingsWidget(parent)
      , Options(GlobalOptions::Instance().Get())
      , Language(UI::Language::Create())
    {
      //setup self
      setupUi(this);
      SetupLanguages();

      Require(connect(languageSelect, SIGNAL(currentIndexChanged(int)), SLOT(OnLanguageChanged(int))));
      using namespace Parameters;
      IntegerValue::Bind(*playlistCachedFiles, *Options, ZXTuneQT::Playlist::Cache::FILES_LIMIT, ZXTuneQT::Playlist::Cache::FILES_LIMIT_DEFAULT);
      IntegerValue::Bind(*playlistCacheLimit, *Options, ZXTuneQT::Playlist::Cache::MEMORY_LIMIT_MB, ZXTuneQT::Playlist::Cache::MEMORY_LIMIT_MB_DEFAULT);
    }

    virtual void OnLanguageChanged(int idx)
    {
      const QString lang = languageSelect->itemData(idx).toString();
      Language->Set(lang);
      Options->SetValue(Parameters::ZXTuneQT::UI::LANGUAGE, FromQString(lang));
    }

    //QWidget
    virtual void changeEvent(QEvent* event)
    {
      if (event && QEvent::LanguageChange == event->type())
      {
        retranslateUi(this);
      }
      UI::InterfaceSettingsWidget::changeEvent(event);
    }
  private:
    void SetupLanguages()
    {
      const QStringList& langs = Language->GetAvailable();
      for (QStringList::const_iterator it = langs.begin(), lim = langs.end(); it != lim; ++it)
      {
        const QString& id(*it);
        const QString& name(QLocale(id).nativeLanguageName());
        languageSelect->addItem(name, id);
      }
      const QString& curLang = GetCurrentLanguage();
      languageSelect->setCurrentIndex(langs.indexOf(curLang));
    }

    QString GetCurrentLanguage() const
    {
      Parameters::StringType val = FromQString(Language->GetSystem());
      Options->FindValue(Parameters::ZXTuneQT::UI::LANGUAGE, val);
      return ToQString(val);
    }
  private:
    const Parameters::Container::Ptr Options;
    const UI::Language::Ptr Language;
  };
}

namespace UI
{
  InterfaceSettingsWidget::InterfaceSettingsWidget(QWidget &parent)
    : QWidget(&parent)
  {
  }

  InterfaceSettingsWidget* InterfaceSettingsWidget::Create(QWidget &parent)
  {
    return new InterfaceOptionsWidget(parent);
  }
}
