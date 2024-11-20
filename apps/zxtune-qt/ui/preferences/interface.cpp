/**
 *
 * @file
 *
 * @brief UI settings pane implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "apps/zxtune-qt/ui/preferences/interface.h"

#include "apps/zxtune-qt/playlist/parameters.h"
#include "apps/zxtune-qt/supp/options.h"
#include "apps/zxtune-qt/ui/desktop/language.h"
#include "apps/zxtune-qt/ui/parameters.h"
#include "apps/zxtune-qt/ui/tools/parameters_helpers.h"
#include "apps/zxtune-qt/ui/utils.h"
#include "apps/zxtune-qt/update/parameters.h"
#include "interface.ui.h"

#include "math/numeric.h"

#include "contract.h"
#include "make_ptr.h"

#include <QtWidgets/QRadioButton>

#include <utility>

namespace
{
  const Parameters::IntType UPDATE_CHECK_PERIODS[] = {
      // never
      0,
      // once a day
      86400,
      // once a week
      86400 * 7,
  };

  class UpdateCheckPeriodComboboxValue : public Parameters::Integer
  {
  public:
    explicit UpdateCheckPeriodComboboxValue(Parameters::Container::Ptr ctr)
      : Ctr(std::move(ctr))
    {}

    int Get() const override
    {
      using namespace Parameters::ZXTuneQT::Update;
      const auto val = Parameters::GetInteger(*Ctr, CHECK_PERIOD, CHECK_PERIOD_DEFAULT);
      const auto* const arrPos = std::find(UPDATE_CHECK_PERIODS, std::end(UPDATE_CHECK_PERIODS), val);
      return arrPos != std::end(UPDATE_CHECK_PERIODS) ? arrPos - UPDATE_CHECK_PERIODS : -1;
    }

    void Set(int val) override
    {
      if (Math::InRange<int>(val, 0, std::size(UPDATE_CHECK_PERIODS) - 1))
      {
        Ctr->SetValue(Parameters::ZXTuneQT::Update::CHECK_PERIOD, UPDATE_CHECK_PERIODS[val]);
      }
    }

    void Reset() override
    {
      Ctr->RemoveValue(Parameters::ZXTuneQT::Update::CHECK_PERIOD);
    }

  private:
    const Parameters::Container::Ptr Ctr;
  };

  class InterfaceOptionsWidget
    : public UI::InterfaceSettingsWidget
    , public UI::Ui_InterfaceSettingsWidget
  {
  public:
    explicit InterfaceOptionsWidget(QWidget& parent)
      : UI::InterfaceSettingsWidget(parent)
      , Options(GlobalOptions::Instance().Get())
      , Language(UI::Language::Create())
    {
      // setup self
      setupUi(this);
      SetupLanguages();

      Require(connect(languageSelect, qOverload<int>(&QComboBox::currentIndexChanged), this,
                      &InterfaceOptionsWidget::OnLanguageChanged));
      using namespace Parameters;
      IntegerValue::Bind(*playlistCachedFiles, *Options, ZXTuneQT::Playlist::Cache::FILES_LIMIT,
                         ZXTuneQT::Playlist::Cache::FILES_LIMIT_DEFAULT);
      IntegerValue::Bind(*playlistCacheLimit, *Options, ZXTuneQT::Playlist::Cache::MEMORY_LIMIT_MB,
                         ZXTuneQT::Playlist::Cache::MEMORY_LIMIT_MB_DEFAULT);
      BooleanValue::Bind(*playlistStoreAllProperties, *Options, ZXTuneQT::Playlist::Store::PROPERTIES,
                         ZXTuneQT::Playlist::Store::PROPERTIES_DEFAULT);
      UpdateCheckPeriod = IntegerValue::Bind(*updateCheckPeriod, MakePtr<UpdateCheckPeriodComboboxValue>(Options));
      BooleanValue::Bind(*appSingleInstance, *Options, ZXTuneQT::SINGLE_INSTANCE, ZXTuneQT::SINGLE_INSTANCE_DEFAULT);
      CmdlineTarget = IntegerValue::Bind(*cmdlineTarget, *Options, ZXTuneQT::Playlist::CMDLINE_TARGET,
                                         ZXTuneQT::Playlist::CMDLINE_TARGET_DEFAULT);
    }

    // QWidget
    void changeEvent(QEvent* event) override
    {
      if (event && QEvent::LanguageChange == event->type())
      {
        const Parameters::ValueSnapshot blockUpdateCheck(*UpdateCheckPeriod);
        const Parameters::ValueSnapshot blockCmdlineTarget(*CmdlineTarget);
        retranslateUi(this);
      }
      UI::InterfaceSettingsWidget::changeEvent(event);
    }

  private:
    void SetupLanguages()
    {
      const QStringList& langs = Language->GetAvailable();
      for (const auto& id : langs)
      {
        const QString& name(QLocale(id).nativeLanguageName());
        languageSelect->addItem(name, id);
      }
      const QString& curLang = GetCurrentLanguage();
      languageSelect->setCurrentIndex(langs.indexOf(curLang));
    }

    QString GetCurrentLanguage() const
    {
      Parameters::StringType val = FromQString(Language->GetSystem());
      Parameters::FindValue(*Options, Parameters::ZXTuneQT::UI::LANGUAGE, val);
      return ToQString(val);
    }

    void OnLanguageChanged(int idx)
    {
      const QString lang = languageSelect->itemData(idx).toString();
      Language->Set(lang);
      Options->SetValue(Parameters::ZXTuneQT::UI::LANGUAGE, FromQString(lang));
    }

  private:
    const Parameters::Container::Ptr Options;
    const UI::Language::Ptr Language;
    Parameters::Value* UpdateCheckPeriod;
    Parameters::Value* CmdlineTarget;
  };
}  // namespace

namespace UI
{
  InterfaceSettingsWidget::InterfaceSettingsWidget(QWidget& parent)
    : QWidget(&parent)
  {}

  InterfaceSettingsWidget* InterfaceSettingsWidget::Create(QWidget& parent)
  {
    return new InterfaceOptionsWidget(parent);
  }
}  // namespace UI
