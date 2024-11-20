/**
 *
 * @file
 *
 * @brief  L10n loading from resources
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "l10n/control.h"

#include "l10n/src/library.h"

#include "binary/data.h"
#include "resource/api.h"
#include "strings/split.h"

#include "string_type.h"

#include <algorithm>
#include <utility>
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
    const auto& elements = Strings::Split(path, R"(/\)"sv);
    if (elements.size() == PATH_ELEMENTS)
    {
      const auto& filename = elements[FILENAME_POS];
      const auto dotPos = filename.find_last_of('.');
      trans.Domain = dotPos == filename.npos ? filename : filename.substr(0, dotPos);
      trans.Language = elements[TRANSLATION_POS];
      trans.Type = dotPos == filename.npos ? StringView() : filename.substr(dotPos + 1);
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
