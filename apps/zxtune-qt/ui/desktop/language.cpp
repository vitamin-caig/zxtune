/*
Abstract:
  UI i18n support implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  This file is a part of zxtune-qt application based on zxtune library
*/

//local includes
#include "language.h"
//library includes
#include <l10n/control.h>
//std includes
#include <map>
#include <set>
//boost includes
#include <boost/bind.hpp>
#include <boost/make_shared.hpp>
//qt includes
#include <QtCore/QCoreApplication>
#include <QtCore/QTranslator>

namespace
{
  class LanguageInResources : public UI::Language
                            , public L10n::Library
  {
  public:
    virtual QStringList GetAvailable() const
    {
      QStringList res;
      for (LangToDumpsSetMap::const_iterator it = Translations.begin(), lim = Translations.end(); it != lim; ++it)
      {
        res.append(QString::fromStdString(it->first));
      }
      return res;
    }

    virtual void Set(const QString& lang)
    {
      SelectTranslation(lang.toStdString());
    }

    //L10n::Library
    virtual void AddTranslation(const L10n::Translation& trans)
    {
      static const std::string QM_FILE("qm");

      if (trans.Type != QM_FILE)
      {
        return L10n::Library::Instance().AddTranslation(trans);
      }
      Translations[trans.Language].insert(DumpPtr(new Dump(trans.Data)));
    }

    virtual void SelectTranslation(const std::string& translation)
    {
      UnloadTranslators();
      const LangToDumpsSetMap::const_iterator it = Translations.find(translation);
      if (it != Translations.end())
      {
        LoadTranslators(it->second);
      }
      L10n::Library::Instance().SelectTranslation(translation);
    }

    virtual L10n::Vocabulary::Ptr GetVocabulary(const std::string& domain) const
    {
      return L10n::Vocabulary::Ptr();
    }
  private:
    typedef boost::shared_ptr<const Dump> DumpPtr;
    typedef std::set<DumpPtr> DumpsSet;
    typedef std::map<std::string, DumpsSet> LangToDumpsSetMap;
    typedef boost::shared_ptr<QTranslator> TranslatorPtr;
    typedef std::set<TranslatorPtr> TranslatorsSet;

    void UnloadTranslators()
    {
      std::for_each(ActiveTranslators.begin(), ActiveTranslators.end(), boost::bind(&QCoreApplication::removeTranslator, boost::bind(&TranslatorPtr::get, _1)));
      ActiveTranslators.clear();
    }

    void LoadTranslators(const DumpsSet& dumps)
    {
      TranslatorsSet newTranslators;
      std::transform(dumps.begin(), dumps.end(), std::inserter(newTranslators, newTranslators.end()), &CreateTranslator);
      std::for_each(newTranslators.begin(), newTranslators.end(), boost::bind(&QCoreApplication::installTranslator, boost::bind(&TranslatorPtr::get, _1)));
      ActiveTranslators.swap(newTranslators);
    }

    static TranslatorPtr CreateTranslator(DumpPtr dump)
    {
      const TranslatorPtr res = boost::make_shared<QTranslator>();
      res->load(&dump->front(), static_cast<int>(dump->size()));
      return res;
    }
  private:
    LangToDumpsSetMap Translations;
    TranslatorsSet ActiveTranslators;
  };
}

namespace UI
{
  Language::Ptr Language::Create()
  {
    boost::shared_ptr<LanguageInResources> res = boost::make_shared<LanguageInResources>();
    L10n::LoadTranslationsFromResources(*res);
    return res;
  }
}
