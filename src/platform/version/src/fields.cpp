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
      String GetFieldValue(const String& fieldName) const override
      {
        if (fieldName == "Program")
        {
          return GetProgramTitle();
        }
        else if (fieldName == "Version")
        {
          return GetProgramVersion();
        }
        else if (fieldName == "Date")
        {
          return GetBuildDate();
        }
        else if (fieldName == "Platform")
        {
          return GetBuildPlatform();
        }
        else if (fieldName == "Arch")
        {
          return GetBuildArchitecture();
        }
        else
        {
          return String();
        }
      }
    };

    std::unique_ptr<Strings::FieldsSource> CreateVersionFieldsSource()
    {
      return std::unique_ptr<Strings::FieldsSource>(new VersionFieldsSource());
    }
  }  // namespace Version
}  // namespace Platform
