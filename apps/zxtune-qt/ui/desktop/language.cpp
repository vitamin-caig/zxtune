/**
 *
 * @file
 *
 * @brief i18n support implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include "language.h"
// common includes
#include <make_ptr.h>
// library includes
#include <l10n/control.h>
// std includes
#include <map>
#include <set>
// qt includes
#include <QtCore/QCoreApplication>
#include <QtCore/QLocale>
#include <QtCore/QTranslator>

namespace
{
  class QtTranslationLibrary : public L10n::Library
  {
  public:
    void AddTranslation(const L10n::Translation& trans) override
    {
      static const std::string QM_FILE("qm");

      if (trans.Type != QM_FILE)
      {
        return L10n::Library::Instance().AddTranslation(trans);
      }
      Translations[trans.Language].insert(DumpPtr(new Binary::Dump(trans.Data)));
    }

    void SelectTranslation(const std::string& translation) override
    {
      UnloadTranslators();
      const LangToDumpsSetMap::const_iterator it = Translations.find(translation);
      if (it != Translations.end())
      {
        LoadTranslators(it->second);
      }
      L10n::Library::Instance().SelectTranslation(translation);
    }

    L10n::Vocabulary::Ptr GetVocabulary(const std::string& /*domain*/) const override
    {
      return L10n::Vocabulary::Ptr();
    }

    std::vector<std::string> EnumerateLanguages() const
    {
      std::vector<std::string> res;
      for (const auto& trans : Translations)
      {
        res.push_back(trans.first);
      }
      return res;
    }

  private:
    typedef std::shared_ptr<const Binary::Dump> DumpPtr;
    typedef std::set<DumpPtr> DumpsSet;
    typedef std::map<std::string, DumpsSet> LangToDumpsSetMap;
    typedef std::shared_ptr<QTranslator> TranslatorPtr;
    typedef std::set<TranslatorPtr> TranslatorsSet;

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
        trans->load(&dump->front(), static_cast<int>(dump->size()));
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
        res << QString::fromStdString(lng);
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
    if (Language::Ptr res = instance.lock())
    {
      return res;
    }
    const Language::Ptr res = MakePtr<LanguageInResources>();
    instance = res;
    return res;
  }
}  // namespace UI
