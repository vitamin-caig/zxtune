/*
Abstract:
  Version fields factory

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#pragma once
#ifndef VERSION_FIELDS_H_DEFINED
#define VERSION_FIELDS_H_DEFINED

//library includes
#include <strings/fields.h>

std::auto_ptr<Strings::FieldsSource> CreateVersionFieldsSource();

#endif //VERSION_FIELDS_H_DEFINED
