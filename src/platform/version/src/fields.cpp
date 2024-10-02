/**
 *
 * @file
 *
 * @brief Version fields source
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include <platform/version/api.h>
#include <platform/version/fields.h>

namespace Platform::Version
{
  class VersionFieldsSource : public Strings::FieldsSource
  {
  public:
    String GetFieldValue(StringView fieldName) const override
    {
      if (fieldName == "Program"sv)
      {
        return GetProgramTitle();
      }
      else if (fieldName == "Version"sv)
      {
        return GetProgramVersion();
      }
      else if (fieldName == "Date"sv)
      {
        return GetBuildDate();
      }
      else if (fieldName == "Platform"sv)
      {
        return GetBuildPlatform();
      }
      else if (fieldName == "Arch"sv)
      {
        return GetBuildArchitecture();
      }
      else
      {
        return {};
      }
    }
  };

  std::unique_ptr<Strings::FieldsSource> CreateVersionFieldsSource()
  {
    return std::unique_ptr<Strings::FieldsSource>(new VersionFieldsSource());
  }
}  // namespace Platform::Version
