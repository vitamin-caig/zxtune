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
  const auto fmtTemplate = Strings::Template::Create(format);
  const auto& emptyTitle = fmtTemplate->Instantiate(Strings::SkipFieldsSource());
  auto curTitle = fmtTemplate->Instantiate(Parameters::FieldsSourceAdapter<Strings::SkipFieldsSource>(props));
  if (curTitle == emptyTitle)
  {
    props.FindValue(Module::ATTR_FULLPATH, curTitle);
  }
  return curTitle;
}
