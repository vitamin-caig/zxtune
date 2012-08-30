/*
Abstract:
  L10n loading from resource

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//library includes
#include <l10n/api.h>
#include <l10n/control.h>
#include <platform/resource.h>
//boost includes
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>

namespace
{
  //$(lang)/$(domain).mo
  enum PathElements
  {
    TRANSLATION_POS = 0,
    FILENAME_POS = 1,

    PATH_ELEMENTS
  };

  const Char PATH_DELIMITERS[] = {'/', '\\', 0};

  struct TranslationResource
  {
    TranslationResource(const std::string& filename)
      : IsValid()
    {
      std::vector<std::string> elements;
      boost::algorithm::split(elements, filename, boost::algorithm::is_any_of(PATH_DELIMITERS), boost::algorithm::token_compress_on);
      if (elements.size() == PATH_ELEMENTS)
      {
        const std::string filename = elements[FILENAME_POS];
        const std::string::size_type dotpos = filename.find_last_of('.');
        if (dotpos != filename.npos)
        {
          IsValid = true;
          Domain = filename.substr(0, dotpos);
          Translation = elements[TRANSLATION_POS];
        }
      }
    }

    bool IsValid;
    std::string Domain;
    std::string Translation;
  };

  Dump LoadResource(const String& name)
  {
    const Binary::Container::Ptr data = Platform::Resource::Load(name);
    const uint8_t* const begin = static_cast<const uint8_t*>(data->Data());
    return Dump(begin, begin + data->Size());
  }

  class ResourceFilesVisitor : public Platform::Resource::Visitor
  {
  public:
    virtual void OnResource(const String& name)
    {
      const TranslationResource res(name);
      if (res.IsValid)
      {
        const Dump data = LoadResource(name);
        L10n::Library::Instance().AddTranslation(res.Domain, res.Translation, data);
      }
    }
  };
}

namespace L10n
{
  void LoadTranslationsFromResources()
  {
    ResourceFilesVisitor visitor;
    Platform::Resource::Enumerate(visitor);
  }
}
