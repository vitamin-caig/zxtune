/**
 *
 * @file
 *
 * @brief  L10n implementation based on boost::locale
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// common includes
#include <contract.h>
#include <make_ptr.h>
// library includes
#include <debug/log.h>
#include <l10n/api.h>
// boost includes
#include <boost/locale/gnu_gettext.hpp>
#include <boost/locale/util.hpp>
// std includes
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
      return it != limit ? &it->second : nullptr;
    }
  };

  typedef std::shared_ptr<std::locale> LocalePtr;

  class DomainVocabulary : public L10n::Vocabulary
  {
  public:
    DomainVocabulary(LocalePtr locale, String domain)
      : Locale(std::move(locale))
      , Domain(std::move(domain))
    {
      Dbg("Created vocabulary for domain '{}'", Domain);
    }

    String GetText(const char* text) const override
    {
      return boost::locale::dgettext<Char>(Domain.c_str(), text, *Locale);
    }

    String GetText(const char* single, const char* plural, int count) const override
    {
      return boost::locale::dngettext<Char>(Domain.c_str(), single, plural, count, *Locale);
    }

    String GetText(const char* context, const char* text) const override
    {
      return boost::locale::dpgettext<Char>(Domain.c_str(), context, text, *Locale);
    }

    String GetText(const char* context, const char* single, const char* plural, int count) const override
    {
      return boost::locale::dnpgettext<Char>(Domain.c_str(), context, single, plural, count, *Locale);
    }

  private:
    const LocalePtr Locale;
    const String Domain;
  };

  // language[_COUNTRY][.encoding][@variant]
  struct LocaleAttributes
  {
    LocaleAttributes()
      : Name(boost::locale::util::get_system_locale(true /*use_utf8_on_windows*/))
    {
      const auto encPos = Name.find_first_of('.');
      if (encPos != Name.npos)
      {
        Translation = Name.substr(0, encPos);
        const auto varPos = Name.find_first_of('@');
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

    const String Name;
    String Translation;
    String Encoding;
  };

  const String TYPE_MO("mo");

  class BoostLocaleLibrary : public L10n::Library
  {
  public:
    BoostLocaleLibrary()
      : SystemLocale()
      , CurrentLocale(new std::locale())
    {
      Dbg("Current locale is {}. Encoding is {}. Translation is {}", SystemLocale.Name, SystemLocale.Encoding,
          SystemLocale.Translation);
    }

    void AddTranslation(const L10n::Translation& trans) override
    {
      if (trans.Type != TYPE_MO)
      {
        return;
      }
      using namespace boost::locale;

      static const String EMPTY_PATH;
      gnu_gettext::messages_info& info = Locales[trans.Language];
      info.language = trans.Language;
      info.encoding = SystemLocale.Encoding;
      info.domains.push_back(gnu_gettext::messages_info::domain(trans.Domain));
      info.callback = &LoadMessage;
      info.paths.assign(&EMPTY_PATH, &EMPTY_PATH + 1);

      const String filename =
          EMPTY_PATH + '/' + trans.Language + '/' + info.locale_category + '/' + trans.Domain + '.' + TYPE_MO;
      Translations[filename] = trans.Data;
      Dbg("Added translation {} in {} bytes", filename, trans.Data.size());
    }

    void SelectTranslation(const String& translation) override
    {
      using namespace boost::locale;
      try
      {
        if (const gnu_gettext::messages_info* info = Locales.Find(translation))
        {
          message_format<Char>* const facet = gnu_gettext::create_messages_facet<Char>(*info);
          Require(facet != nullptr);
          *CurrentLocale = std::locale(std::locale::classic(), facet);
          Dbg("Selected translation {}", translation);
          return;
        }
      }
      catch (const std::exception& e)
      {
        Dbg("Failed to select translation {}: {}", e.what());
      }
      *CurrentLocale = std::locale();
      Dbg("Selected unknown translation {}", translation);
    }

    L10n::Vocabulary::Ptr GetVocabulary(const String& domain) const override
    {
      return MakePtr<DomainVocabulary>(CurrentLocale, domain);
    }

    static BoostLocaleLibrary& Instance()
    {
      static BoostLocaleLibrary self;
      return self;
    }

  private:
    static std::vector<char> LoadMessage(const String& file, const String& encoding)
    {
      const BoostLocaleLibrary& self = Instance();
      if (const auto* data = self.Translations.Find(file))
      {
        Dbg("Loading message {} with encoding {}", file, encoding);
        return std::vector<char>(data->begin(), data->end());
      }
      else
      {
        Dbg("Message {} with encoding {} not found", file, encoding);
        return std::vector<char>();
      }
    }

  private:
    const LocaleAttributes SystemLocale;
    const LocalePtr CurrentLocale;
    MapAdapter<String, Binary::Dump> Translations;
    MapAdapter<String, boost::locale::gnu_gettext::messages_info> Locales;
  };
}  // namespace

namespace L10n
{
  Library& Library::Instance()
  {
    return BoostLocaleLibrary::Instance();
  }
}  // namespace L10n
