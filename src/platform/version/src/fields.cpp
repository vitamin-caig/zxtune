/**
* 
* @file
*
* @brief Version fields source
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include <platform/version/api.h>
#include <platform/version/fields.h>
//text includes
#include <platform/version/text/text.h>

namespace Platform
{
  namespace Version
  {
    class VersionFieldsSource : public Strings::FieldsSource
    {
    public:
      String GetFieldValue(const String& fieldName) const override
      {
        if (fieldName == Text::FIELD_PROGRAM_NAME)
        {
          return GetProgramTitle();
        }
        else if (fieldName == Text::FIELD_PROGRAM_VERSION)
        {
          return GetProgramVersion();
        }
        else if (fieldName == Text::FIELD_BUILD_DATE)
        {
          return GetBuildDate();
        }
        else if (fieldName == Text::FIELD_BUILD_PLATFORM)
        {
          return GetBuildPlatform();
        }
        else if (fieldName == Text::FIELD_BUILD_ARCH)
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
  }
}
