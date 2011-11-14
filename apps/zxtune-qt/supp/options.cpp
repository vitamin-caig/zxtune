/*
Abstract:
  Options accessing implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  This file is a part of zxtune-qt application based on zxtune library
*/

//local includes
#include "options.h"
#include <apps/base/parsing.h>

namespace
{
  class GlobalOptionsStorage : public GlobalOptions
  {
  public:
    virtual void LoadDefault()
    {
      ReadFromConfig();
    }

    virtual Parameters::Container::Ptr Get() const
    {
      if (!Options)
      {
        ReadFromConfig();
      }
      return Options;
    }
  private:
    void ReadFromConfig() const
    {
      const Parameters::Container::Ptr result = Parameters::Container::Create();
      ThrowIfError(ParseConfigFile(String(), *result));
      Options = result;
    }
  private:
    mutable Parameters::Container::Ptr Options;
  };
}

GlobalOptions& GlobalOptions::Instance()
{
  static GlobalOptionsStorage storage;
  return storage;
}
