/**
 *
 * @file
 *
 * @brief Format tools implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// library includes
#include <module/attributes.h>
#include <parameters/template.h>
#include <strings/template.h>

String GetModuleTitle(StringView format, const Parameters::Accessor& props)
{
  // TODO: remove duplicate of this logic
  const auto fmtTemplate = Strings::Template::Create(format);
  const auto& emptyTitle = fmtTemplate->Instantiate(Strings::SkipFieldsSource());
  auto curTitle = fmtTemplate->Instantiate(Parameters::FieldsSourceAdapter<Strings::SkipFieldsSource>(props));
  if (curTitle == emptyTitle)
  {
    curTitle = Parameters::GetString(props, Module::ATTR_FULLPATH);
  }
  return curTitle;
}
