/**
*
* @file
*
* @brief  L10n loading from resources
*
* @author vitamin.caig@gmail.com
*
**/

//library includes
#include <l10n/api.h>
#include <l10n/control.h>
#include <resource/api.h>
//boost includes
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/split.hpp>

namespace
{
  //$(lang)/$(domain)[.$(type)]
  enum PathElements
  {
    TRANSLATION_POS = 0,
    FILENAME_POS = 1,

    PATH_ELEMENTS
  };

  bool ParseFilename(const std::string& path, L10n::Translation& trans)
  {
    std::vector<std::string> elements;
    static const std::string PATH_DELIMITERS("/\\");
    boost::algorithm::split(elements, path, boost::algorithm::is_any_of(PATH_DELIMITERS), boost::algorithm::token_compress_on);
    if (elements.size() == PATH_ELEMENTS)
    {
      const std::string filename = elements[FILENAME_POS];
      const std::string::size_type dotPos = filename.find_last_of('.');
      trans.Domain = dotPos == filename.npos ? filename : filename.substr(0, dotPos);
      trans.Language = elements[TRANSLATION_POS];
      trans.Type = dotPos == filename.npos ? std::string() : filename.substr(dotPos + 1);
      return true;
    }
    return false;
  }

  Dump LoadResource(const String& name)
  {
    const Binary::Container::Ptr data = Resource::Load(name);
    const uint8_t* const begin = static_cast<const uint8_t*>(data->Start());
    return Dump(begin, begin + data->Size());
  }

  class ResourceFilesVisitor : public Resource::Visitor
  {
  public:
    explicit ResourceFilesVisitor(L10n::Library& lib)
      : Lib(lib)
    {
    }

    virtual void OnResource(const String& name)
    {
      L10n::Translation trans;
      if (ParseFilename(name, trans))
      {
        trans.Data = LoadResource(name);
        Lib.AddTranslation(trans);
      }
    }
  private:
    L10n::Library& Lib;
  };
}

namespace L10n
{
  void LoadTranslationsFromResources(Library& lib)
  {
    ResourceFilesVisitor visitor(lib);
    Resource::Enumerate(visitor);
  }
}
