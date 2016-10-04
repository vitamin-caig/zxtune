/**
*
* @file
*
* @brief  L10n implementation based on boost::locale
*
* @author vitamin.caig@gmail.com
*
**/

//common includes
#include <contract.h>
#include <make_ptr.h>
//library includes
#include <debug/log.h>
#include <l10n/api.h>
//boost includes
#include <boost/locale/gnu_gettext.hpp>
#include <boost/locale/util.hpp>
//std includes
#include <map>

namespace
{
  const Debug::Stream Dbg("L10n");

  template<class K, class V>
  class MapAdapter : public std::map<K, V>
  {
  public:
    const V* Find(const K& key) const
    {
      const typename std::map<K, V>::const_iterator it = std::map<K, V>::find(key), limit = std::map<K, V>::end();
      return it != limit
        ? &it->second
        : 0;
    }
  };

  typedef std::shared_ptr<std::locale> LocalePtr;

  class DomainVocabulary : public L10n::Vocabulary
  {
  public:
    DomainVocabulary(LocalePtr locale, const std::string& domain)
      : Locale(locale)
      , Domain(domain)
    {
      Dbg("Created vocabulary for domain '%1%'", domain);
    }

    virtual String GetText(const char* text) const
    {
      return boost::locale::dgettext<Char>(Domain.c_str(), text, *Locale);
    }

    virtual String GetText(const char* single, const char* plural, int count) const
    {
      return boost::locale::dngettext<Char>(Domain.c_str(), single, plural, count, *Locale);
    }

    virtual String GetText(const char* context, const char* text) const
    {
      return boost::locale::dpgettext<Char>(Domain.c_str(), context, text, *Locale);
    }

    virtual String GetText(const char* context, const char* single, const char* plural, int count) const
    {
      return boost::locale::dnpgettext<Char>(Domain.c_str(), context, single, plural, count, *Locale);
    }
  private:
    const LocalePtr Locale;
    const std::string Domain;
  };
  
  //language[_COUNTRY][.encoding][@variant]
  struct LocaleAttributes
  {
    LocaleAttributes()
      : Name(boost::locale::util::get_system_locale())
    {
      const std::string::size_type encPos = Name.find_first_of('.');
      if (encPos != Name.npos)
      {
        Translation = Name.substr(0, encPos);
        const std::string::size_type varPos = Name.find_first_of('@');
        if (varPos != Name.npos)
        {
          Encoding = Name.substr(encPos + 1, varPos - encPos - 1);
        }
        else
        {
          Encoding = Name.substr(encPos + 1);
        }
      }
    }

    const std::string Name;
    std::string Translation;
    std::string Encoding;
  };

  const std::string TYPE_MO("mo");

  class BoostLocaleLibrary : public L10n::Library
  {
  public:
    BoostLocaleLibrary()
      : SystemLocale()
      , CurrentLocale(new std::locale())
    {
      Dbg("Current locale is %1%. Encoding is %2%. Translation is %3%", SystemLocale.Name, SystemLocale.Encoding, SystemLocale.Translation);
    }

    virtual void AddTranslation(const L10n::Translation& trans)
    {
      if (trans.Type != TYPE_MO)
      {
        return;
      }
      using namespace boost::locale;

      static const std::string EMPTY_PATH;
      gnu_gettext::messages_info& info = Locales[trans.Language];
      info.language = trans.Language;
      info.encoding = SystemLocale.Encoding;
      info.domains.push_back(gnu_gettext::messages_info::domain(trans.Domain));
      info.callback = &LoadMessage;
      info.paths.assign(&EMPTY_PATH, &EMPTY_PATH + 1);

      const std::string filename = EMPTY_PATH + '/' + trans.Language + '/' + info.locale_category + '/' + trans.Domain + '.' + TYPE_MO;
      Translations[filename] = trans.Data;
      Dbg("Added translation %1% in %2% bytes", filename, trans.Data.size());
    }

    virtual void SelectTranslation(const std::string& translation)
    {
      using namespace boost::locale;
      try
      {
        if (const gnu_gettext::messages_info* info = Locales.Find(translation))
        {
          message_format<Char>* const facet = gnu_gettext::create_messages_facet<Char>(*info);
          Require(facet != 0);
          *CurrentLocale = std::locale(std::locale::classic(), facet);
          Dbg("Selected translation %1%", translation);
          return;
        }
      }
      catch (const std::exception& e)
      {
        Dbg("Failed to select translation %1%: %2%", e.what());
      }
      *CurrentLocale = std::locale();
      Dbg("Selected unknown translation %1%", translation);
    }

    virtual L10n::Vocabulary::Ptr GetVocabulary(const std::string& domain) const
    {
      return MakePtr<DomainVocabulary>(CurrentLocale, domain);
    }

    static BoostLocaleLibrary& Instance()
    {
      static BoostLocaleLibrary self;
      return self;
    }
  private:
    static std::vector<char> LoadMessage(const std::string& file, const std::string& encoding)
    {
      const BoostLocaleLibrary& self = Instance();
      if (const Dump* data = self.Translations.Find(file))
      {
        Dbg("Loading message %1% with encoding %2%", file, encoding);
        return std::vector<char>(data->begin(), data->end());
      }
      else
      {
        Dbg("Message %1% with encoding %2% not found", file, encoding);
        return std::vector<char>();
      }
    }
  private:
    const LocaleAttributes SystemLocale;
    const LocalePtr CurrentLocale;
    MapAdapter<std::string, Dump> Translations;
    MapAdapter<std::string, boost::locale::gnu_gettext::messages_info> Locales;
  };
}

namespace L10n
{
  Library& Library::Instance()
  {
    return BoostLocaleLibrary::Instance();
  }
}
