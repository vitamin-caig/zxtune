/*
Abstract:
  Source data provider declaration and related functions

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  This file is a part of zxtune123 application based on zxtune library
*/

#pragma once
#ifndef ZXTUNE123_SOURCE_H_DEFINED
#define ZXTUNE123_SOURCE_H_DEFINED

//library includes
#include <core/module_holder.h>
//std includes
#include <memory>
#include <stdexcept>
//boost includes
#include <boost/function.hpp>

//forward declarations
namespace boost
{
  namespace program_options
  {
    class options_description;
  }
}

class CancelError : public std::exception {};

typedef boost::function<void(ZXTune::Module::Holder::Ptr)> OnItemCallback;

class SourceComponent
{
public:
  virtual ~SourceComponent() {}
  // commandline-related part
  virtual const boost::program_options::options_description& GetOptionsDescription() const = 0;
  virtual void ParseParameters() = 0;
  // throw
  virtual void Initialize() = 0;
  virtual void ProcessItems(const OnItemCallback& callback) = 0;

  static std::auto_ptr<SourceComponent> Create(Parameters::Container::Ptr configParams);
};

#endif //ZXTUNE123_SOURCE_H_DEFINED
