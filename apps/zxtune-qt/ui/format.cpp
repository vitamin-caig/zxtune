/**
 *
 * @file
 *
 * @brief Format tools implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "module/attributes.h"
#include "parameters/accessor.h"
#include "parameters/template.h"
#include "strings/fields.h"
#include "strings/template.h"

#include "string_type.h"
#include "string_view.h"

#include <memory>

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
