/**
* 
* @file
*
* @brief Format tools implementation
*
* @author vitamin.caig@gmail.com
*
**/

//library includes
#include <core/module_attrs.h>
#include <core/module_types.h>
#include <parameters/template.h>
#include <strings/template.h>

String GetModuleTitle(const String& format, const Parameters::Accessor& props)
{
  const Strings::Template::Ptr fmtTemplate = Strings::Template::Create(format);
  const String& emptyTitle = fmtTemplate->Instantiate(Strings::SkipFieldsSource());
  String curTitle = fmtTemplate->Instantiate(Parameters::FieldsSourceAdapter<Strings::SkipFieldsSource>(props));
  if (curTitle == emptyTitle)
  {
    props.FindValue(Module::ATTR_FULLPATH, curTitle);
  }
  return curTitle;
}
