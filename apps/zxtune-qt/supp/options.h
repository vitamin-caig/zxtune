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

#include "parameters/container.h"

class GlobalOptions
{
public:
  virtual ~GlobalOptions() = default;

  virtual Parameters::Container::Ptr Get() const = 0;
  virtual Parameters::Accessor::Ptr GetSnapshot() const = 0;

  static GlobalOptions& Instance();
};
