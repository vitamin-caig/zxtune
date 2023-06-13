/**
 *
 * @file
 *
 * @brief  L10n loading from resources
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// library includes
#include <l10n/api.h>
#include <l10n/control.h>
#include <resource/api.h>
#include <strings/split.h>
// std includes
#include <vector>

namespace
{
  //$(lang)/$(domain)[.$(type)]
  enum PathElements
  {
    TRANSLATION_POS = 0,
    FILENAME_POS = 1,

    PATH_ELEMENTS
  };

  bool ParseFilename(StringView path, L10n::Translation& trans)
  {
    std::vector<StringView> elements;
    Strings::Split(path, "/\\"_sv, elements);
    if (elements.size() == PATH_ELEMENTS)
    {
      const auto& filename = elements[FILENAME_POS];
      const auto dotPos = filename.find_last_of('.');
      trans.Domain = (dotPos == filename.npos ? filename : filename.substr(0, dotPos)).to_string();
      trans.Language = elements[TRANSLATION_POS].to_string();
      trans.Type = dotPos == filename.npos ? String() : filename.substr(dotPos + 1).to_string();
      return true;
    }
    return false;
  }

  Binary::Data::Ptr LoadResource(StringView name)
  {
    return Resource::Load(name);
  }

  class ResourceFilesVisitor : public Resource::Visitor
  {
  public:
    explicit ResourceFilesVisitor(L10n::Library& lib)
      : Lib(lib)
    {}

    void OnResource(StringView name) override
    {
      L10n::Translation trans;
      if (ParseFilename(name, trans))
      {
        trans.Data = LoadResource(name);
        Lib.AddTranslation(std::move(trans));
      }
    }

  private:
    L10n::Library& Lib;
  };
}  // namespace

namespace L10n
{
  void LoadTranslationsFromResources(Library& lib)
  {
    ResourceFilesVisitor visitor(lib);
    Resource::Enumerate(visitor);
  }
}  // namespace L10n
