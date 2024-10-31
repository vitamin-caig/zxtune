/**
 *
 * @file
 *
 * @brief i18n support implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "apps/zxtune-qt/ui/desktop/language.h"

#include "apps/zxtune-qt/ui/utils.h"

#include "l10n/src/library.h"

#include "binary/data.h"
#include "l10n/control.h"

#include "make_ptr.h"
#include "string_type.h"
#include "string_view.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QFunctionPointer>
#include <QtCore/QLatin1Char>
#include <QtCore/QLocale>
#include <QtCore/QTranslator>

#include <algorithm>
#include <functional>
#include <map>
#include <set>
#include <utility>
#include <vector>

namespace
{
  class QtTranslationLibrary : public L10n::Library
  {
  public:
    void AddTranslation(L10n::Translation trans) override
    {
      const auto QM_FILE = "qm"sv;

      if (trans.Type != QM_FILE)
      {
        return L10n::Library::Instance().AddTranslation(std::move(trans));
      }
      Translations[trans.Language].insert(std::move(trans.Data));
    }

    void SelectTranslation(StringView translation) override
    {
      UnloadTranslators();
      const auto it = Translations.find(translation);
      if (it != Translations.end())
      {
        LoadTranslators(it->second);
      }
      L10n::Library::Instance().SelectTranslation(translation);
    }

    L10n::Vocabulary::Ptr GetVocabulary(StringView /*domain*/) const override
    {
      return {};
    }

    std::vector<StringView> EnumerateLanguages() const
    {
      std::vector<StringView> res;
      for (const auto& trans : Translations)
      {
        res.push_back(trans.first);
      }
      return res;
    }

  private:
    using DumpsSet = std::set<Binary::Data::Ptr>;
    using LangToDumpsSetMap = std::map<String, DumpsSet, std::less<>>;
    using TranslatorPtr = std::shared_ptr<QTranslator>;
    using TranslatorsSet = std::set<TranslatorPtr>;

    void UnloadTranslators()
    {
      for (const auto& trans : ActiveTranslators)
      {
        QCoreApplication::removeTranslator(trans.get());
      }
      ActiveTranslators.clear();
    }

    void LoadTranslators(const DumpsSet& dumps)
    {
      TranslatorsSet newTranslators;
      for (const auto& dump : dumps)
      {
        auto trans = std::make_shared<QTranslator>();
        trans->load(static_cast<const uchar*>(dump->Start()), static_cast<int>(dump->Size()));
        QCoreApplication::installTranslator(trans.get());
        newTranslators.emplace(std::move(trans));
      }
      ActiveTranslators.swap(newTranslators);
    }

  private:
    LangToDumpsSetMap Translations;
    TranslatorsSet ActiveTranslators;
  };

  class LanguageInResources : public UI::Language
  {
  public:
    LanguageInResources()
    {
      L10n::LoadTranslationsFromResources(Lib);
    }

    QStringList GetAvailable() const override
    {
      QStringList res;
      res << QLatin1String("en");  // by default
      for (const auto& lng : Lib.EnumerateLanguages())
      {
        res << ToQString(lng);
      }
      return res;
    }

    QString GetSystem() const override
    {
      QString curLang = QLocale::system().name();
      curLang.truncate(curLang.lastIndexOf(QLatin1Char('_')));  //$(lang)_$(country)
      return curLang;
    }

    void Set(const QString& lang) override
    {
      if (lang != Lang)
      {
        Lib.SelectTranslation(lang.toStdString());
        const QLocale locale(lang);
        QLocale::setDefault(locale);
        Lang = lang;
      }
    }

  private:
    QtTranslationLibrary Lib;
    QString Lang;
  };
}  // namespace

namespace UI
{
  Language::Ptr Language::Create()
  {
    // use slight caching to prevent heavy parsing
    static std::weak_ptr<Language> instance;
    if (auto res = instance.lock())
    {
      return res;
    }
    auto res = MakePtr<LanguageInResources>();
    instance = res;
    return res;
  }
}  // namespace UI
