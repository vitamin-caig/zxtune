/*
Abstract:
  Options accessing interface

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  This file is a part of zxtune-qt application based on zxtune library
*/

#pragma once
#ifndef ZXTUNE_QT_OPTIONS_H_DEFINED
#define ZXTUNE_QT_OPTIONS_H_DEFINED

//library includes
#include <parameters/container.h>

class GlobalOptions
{
public:
  virtual ~GlobalOptions() {}

  virtual Parameters::Container::Ptr Get() const = 0;

  static GlobalOptions& Instance();
};

#endif //ZXTUNE_QT_OPTIONS_H_DEFINED
