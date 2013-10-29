/**
* 
* @file
*
* @brief Options access interface
*
* @author vitamin.caig@gmail.com
*
**/

#pragma once

//library includes
#include <parameters/container.h>

class GlobalOptions
{
public:
  virtual ~GlobalOptions() {}

  virtual Parameters::Container::Ptr Get() const = 0;
  virtual Parameters::Accessor::Ptr GetSnapshot() const = 0;

  static GlobalOptions& Instance();
};
