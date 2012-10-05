/*
Abstract:
  Formatting tools implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  This file is a part of zxtune-qt application based on zxtune library
*/

//common includes
#include <template_parameters.h>
//library includes
#include <core/module_attrs.h>
#include <core/module_types.h>
#include <strings/template.h>

String GetModuleTitle(const String& format, const Parameters::Accessor& props)
{
  const Strings::Template::Ptr fmtTemplate = Strings::Template::Create(format);
  const String& emptyTitle = fmtTemplate->Instantiate(Strings::SkipFieldsSource());
  String curTitle = fmtTemplate->Instantiate(Parameters::FieldsSourceAdapter<Strings::SkipFieldsSource>(props));
  if (curTitle == emptyTitle)
  {
    props.FindValue(ZXTune::Module::ATTR_FULLPATH, curTitle);
  }
  return curTitle;
}
