/*
Abstract:
  Informational component declaration

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  This file is a part of zxtune123 application based on zxtune library
*/

#pragma once
#ifndef ZXTUNE123_INFORMATION_H_DEFINED
#define ZXTUNE123_INFORMATION_H_DEFINED

//std includes
#include <memory>

//forward declaration
namespace boost
{
  namespace program_options
  {
    class options_description;
  }
}

class InformationComponent
{
public:
  virtual ~InformationComponent() {}
  // commandline-related part
  virtual const boost::program_options::options_description& GetOptionsDescription() const = 0;
  //return true if should exit
  virtual bool Process() const = 0;

  static std::auto_ptr<InformationComponent> Create();
};

#endif //ZXTUNE123_SOUND_H_DEFINED
