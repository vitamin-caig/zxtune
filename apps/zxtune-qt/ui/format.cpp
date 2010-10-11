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

String GetModuleTitle(const String& format, const Parameters::Accessor& props)
{
  const StringTemplate::Ptr fmtTemplate = StringTemplate::Create(format);
  const String& emptyTitle = fmtTemplate->Instantiate(SkipFieldsSource());
  String curTitle = fmtTemplate->Instantiate(Parameters::FieldsSourceAdapter<SkipFieldsSource>(props));
  if (curTitle == emptyTitle)
  {
    props.FindStringValue(ZXTune::Module::ATTR_FULLPATH, curTitle);
  }
  return curTitle;
}
