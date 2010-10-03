/*
Abstract:
  Formatting tools implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  This file is a part of zxtune-qt application based on zxtune library
*/

//local includes
#include "format.h"
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

// version definition-related
#ifndef APPLICATION_NAME
#define APPLICATION_NAME zxtune-qt
#endif
#ifndef ZXTUNE_VERSION
#define ZXTUNE_VERSION develop
#endif

#define STR(a) #a
#define MAKE_VERSION_STRING(app, ver) STR(app) " " STR(ver)

String GetProgramTitle()
{
  static const std::string PROGRAM_TITLE(
    MAKE_VERSION_STRING(APPLICATION_NAME, ZXTUNE_VERSION));
  return FromStdString(PROGRAM_TITLE);
}



