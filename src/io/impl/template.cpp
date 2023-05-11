/**
 *
 * @file
 *
 * @brief  Filename template implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include "io/impl/filesystem_path.h"
// common includes
#include <make_ptr.h>
// library includes
#include <io/template.h>
#include <strings/fields.h>

namespace IO
{
  class FilenameFieldsFilter : public Strings::FieldsSource
  {
  public:
    explicit FilenameFieldsFilter(const Strings::FieldsSource& delegate)
      : Delegate(delegate)
    {}

    String GetFieldValue(StringView fieldName) const override
    {
      auto res = Delegate.GetFieldValue(fieldName);
      return res.empty() ? res : FilterPath(res);
    }

  private:
    static String FilterPath(StringView val)
    {
      const auto path = Details::FromString(val);
      const auto root = path.root_directory();
      const auto thisDir = "."_sv;
      const auto parentDir = ".."_sv;
      String res;
      for (const auto& it : path)
      {
        // root directory is usually mentioned while iterations. For windows-based platforms it can be placed not on the
        // first position
        if (it == root)
        {
          continue;
        }
        const auto elem = Details::ToString(it);
        if (elem == thisDir || elem == parentDir)
        {
          continue;
        }
        if (!res.empty())
        {
          res += '_';
        }
        res += elem;
      }
      return res;
    }

  private:
    const Strings::FieldsSource& Delegate;
  };

  class FilenameTemplate : public Strings::Template
  {
  public:
    explicit FilenameTemplate(Strings::Template::Ptr delegate)
      : Delegate(std::move(delegate))
    {}

    String Instantiate(const Strings::FieldsSource& source) const override
    {
      const FilenameFieldsFilter filter(source);
      return Delegate->Instantiate(filter);
    }

  private:
    const Strings::Template::Ptr Delegate;
  };

  Strings::Template::Ptr CreateFilenameTemplate(StringView notation)
  {
    auto delegate = Strings::Template::Create(notation);
    return MakePtr<FilenameTemplate>(std::move(delegate));
  }
}  // namespace IO
