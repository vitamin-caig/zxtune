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
#include <strings/map.h>
// boost includes
#include <boost/locale/gnu_gettext.hpp>
#include <boost/locale/util.hpp>

namespace
{
  const Debug::Stream Dbg("L10n");

  using LocalePtr = std::shared_ptr<std::locale>;

  class DomainVocabulary : public L10n::Vocabulary
  {
  public:
    DomainVocabulary(LocalePtr locale, StringView domain)
      : Locale(std::move(locale))
      , Domain(domain.to_string())
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

    void AddTranslation(L10n::Translation trans) override
    {
      if (trans.Type != TYPE_MO)
      {
        return;
      }
      using namespace boost::locale;

      static const String EMPTY_PATH;
      auto& info = Locales[trans.Language];
      info.language = trans.Language;
      info.encoding = SystemLocale.Encoding;
      info.domains.emplace_back(trans.Domain);
      info.callback = &LoadMessage;
      info.paths.assign(&EMPTY_PATH, &EMPTY_PATH + 1);

      const auto filename =
          EMPTY_PATH + '/' + trans.Language + '/' + info.locale_category + '/' + trans.Domain + '.' + TYPE_MO;
      Dbg("Added translation {} in {} bytes", filename, trans.Data->Size());
      Translations.emplace(filename, std::move(trans.Data));
    }

    void SelectTranslation(StringView translation) override
    {
      using namespace boost::locale;
      try
      {
        if (const auto* info = Locales.FindPtr(translation))
        {
          auto* const facet = gnu_gettext::create_messages_facet<Char>(*info);
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

    L10n::Vocabulary::Ptr GetVocabulary(StringView domain) const override
    {
      return MakePtr<DomainVocabulary>(CurrentLocale, domain);
    }

    static BoostLocaleLibrary& Instance()
    {
      static BoostLocaleLibrary self;
      return self;
    }

  private:
    static std::vector<char> LoadMessage(StringView file, StringView encoding)
    {
      const BoostLocaleLibrary& self = Instance();
      if (const auto* data = self.Translations.FindPtr(file))
      {
        Dbg("Loading message {} with encoding {}", file, encoding);
        const auto* rawStart = static_cast<const char*>((*data)->Start());
        const auto rawSize = (*data)->Size();
        return {rawStart, rawStart + rawSize};
      }
      else
      {
        Dbg("Message {} with encoding {} not found", file, encoding);
        return {};
      }
    }

  private:
    const LocaleAttributes SystemLocale;
    const LocalePtr CurrentLocale;
    Strings::ValueMap<Binary::Data::Ptr> Translations;
    Strings::ValueMap<boost::locale::gnu_gettext::messages_info> Locales;
  };
}  // namespace

namespace L10n
{
  Library& Library::Instance()
  {
    return BoostLocaleLibrary::Instance();
  }
}  // namespace L10n
