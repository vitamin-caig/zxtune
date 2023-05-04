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

namespace Platform
{
  namespace Version
  {
    class VersionFieldsSource : public Strings::FieldsSource
    {
    public:
      String GetFieldValue(StringView fieldName) const override
      {
        if (fieldName == "Program"_sv)
        {
          return GetProgramTitle();
        }
        else if (fieldName == "Version"_sv)
        {
          return GetProgramVersion();
        }
        else if (fieldName == "Date"_sv)
        {
          return GetBuildDate();
        }
        else if (fieldName == "Platform"_sv)
        {
          return GetBuildPlatform();
        }
        else if (fieldName == "Arch"_sv)
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
  }  // namespace Version
}  // namespace Platform
