/*
Abstract:
  L10n stub implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//common includes
#include <tools.h>
//library includes
#include <l10n/api.h>

namespace
{
  class StubVocabulary : public L10n::Vocabulary
  {
  public:
    virtual String GetText(const char* text) const
    {
      return FromStdString(std::string(text));
    }

    virtual String GetText(const char* single, const char* plural, int count) const
    {
      return FromStdString(std::string(count == 1 ? single : plural));
    }

    virtual String GetText(const char* /*context*/, const char* text) const
    {
      return FromStdString(std::string(text));
    }

    virtual String GetText(const char* /*context*/, const char* single, const char* plural, int count) const
    {
      return FromStdString(std::string(count == 1 ? single : plural));
    }
  };

  class StubLibrary : public L10n::Library
  {
  public:
    virtual void AddTranslation(const std::string& /*domain*/, const std::string& /*translation*/, const Dump& /*data*/)
    {
    }

    virtual void SelectTranslation(const std::string& /*translation*/)
    {
    }

    virtual L10n::Vocabulary::Ptr GetVocabulary(const std::string& /*domain*/) const
    {
      static StubVocabulary voc;
      return L10n::Vocabulary::Ptr(&voc, NullDeleter<StubVocabulary>());
    }
  };
}

namespace L10n
{
  Library& Library::Instance()
  {
    static StubLibrary instance;
    return instance;
  }

  void LoadTranslationsFromResources()
  {
  }
}
