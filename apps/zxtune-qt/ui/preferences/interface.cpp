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

#include "contract.h"

#include <QtWidgets/QRadioButton>

#include <utility>
#include <vector>

namespace
{
  const std::vector<Parameters::IntType> UPDATE_CHECK_PERIODS = {
      // never
      0,
      // once a day
      86400,
      // once a week
      86400 * 7,
  };

}  // namespace

namespace UI
{
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
      Value::Bind<ZXTuneQT::Playlist::Cache::FilesLimit>(*playlistCachedFiles, *Options);
      Value::Bind<ZXTuneQT::Playlist::Cache::MemoryLimitMb>(*playlistCacheLimit, *Options);
      Value::Bind<ZXTuneQT::Playlist::Store::Properties>(*playlistStoreAllProperties, *Options);
      UpdateCheckPeriod =
          Value::Bind<ZXTuneQT::Update::CheckPeriod>(*updateCheckPeriod, *Options, UPDATE_CHECK_PERIODS);
      Value::Bind<ZXTuneQT::SingleInstance>(*appSingleInstance, *Options);
      CmdlineTarget = Value::Bind<ZXTuneQT::Playlist::CmdlineTarget>(*cmdlineTarget, *Options);
    }

    // QWidget
    void changeEvent(QEvent* event) override
    {
      if (event && QEvent::LanguageChange == event->type())
      {
        const ValueSnapshot blockUpdateCheck(*UpdateCheckPeriod);
        const ValueSnapshot blockCmdlineTarget(*CmdlineTarget);
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
      if (const auto stored = Parameters::Find<Parameters::ZXTuneQT::UI::Language>(*Options))
      {
        val = *stored;
      }
      return ToQString(val);
    }

    void OnLanguageChanged(int idx)
    {
      const QString lang = languageSelect->itemData(idx).toString();
      Language->Set(lang);
      Parameters::Set<Parameters::ZXTuneQT::UI::Language>(*Options, FromQString(lang));
    }

  private:
    const Parameters::Container::Ptr Options;
    const UI::Language::Ptr Language;
    Value* UpdateCheckPeriod;
    Value* CmdlineTarget;
  };
  InterfaceSettingsWidget::InterfaceSettingsWidget(QWidget& parent)
    : QWidget(&parent)
  {}

  InterfaceSettingsWidget* InterfaceSettingsWidget::Create(QWidget& parent)
  {
    return new InterfaceOptionsWidget(parent);
  }
}  // namespace UI
