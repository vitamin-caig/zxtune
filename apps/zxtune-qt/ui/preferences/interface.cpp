/**
* 
* @file
*
* @brief UI settings pane implementation
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "interface.h"
#include "interface.ui.h"
#include "playlist/parameters.h"
#include "supp/options.h"
#include "ui/utils.h"
#include "ui/parameters.h"
#include "ui/desktop/language.h"
#include "ui/tools/parameters_helpers.h"
#include "update/parameters.h"
//common includes
#include <contract.h>
//library includes
#include <math/numeric.h>
//boost includes
#include <boost/make_shared.hpp>
#include <boost/range/end.hpp>
#include <boost/range/size.hpp>
//qt includes
#include <QtGui/QRadioButton>

namespace
{
  const Parameters::IntType UPDATE_CHECK_PERIODS[] =
  {
    //never
    0,
    //once a day
    86400,
    //once a week
    86400 * 7,
  };

  class UpdateCheckPeriodComboboxValue : public Parameters::Integer
  {
  public:
    explicit UpdateCheckPeriodComboboxValue(Parameters::Container::Ptr ctr)
      : Ctr(ctr)
    {
    }

    virtual int Get() const
    {
      using namespace Parameters;
      Parameters::IntType val = ZXTuneQT::Update::CHECK_PERIOD_DEFAULT;
      Ctr->FindValue(ZXTuneQT::Update::CHECK_PERIOD, val);
      const Parameters::IntType* const arrPos = std::find(UPDATE_CHECK_PERIODS, boost::end(UPDATE_CHECK_PERIODS), val);
      return arrPos != boost::end(UPDATE_CHECK_PERIODS)
        ? arrPos - UPDATE_CHECK_PERIODS
        : -1;
    }

    virtual void Set(int val)
    {
      if (Math::InRange<int>(val, 0, boost::size(UPDATE_CHECK_PERIODS) - 1))
      {
        Ctr->SetValue(Parameters::ZXTuneQT::Update::CHECK_PERIOD, UPDATE_CHECK_PERIODS[val]);
      }
    }

    virtual void Reset()
    {
      Ctr->RemoveValue(Parameters::ZXTuneQT::Update::CHECK_PERIOD);
    }
  private:
    const Parameters::Container::Ptr Ctr;
  };

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
      BooleanValue::Bind(*playlistStoreAllProperties, *Options, ZXTuneQT::Playlist::Store::PROPERTIES, ZXTuneQT::Playlist::Store::PROPERTIES_DEFAULT);
      IntegerValue::Bind(*updateCheckPeriod, boost::make_shared<UpdateCheckPeriodComboboxValue>(Options));
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
